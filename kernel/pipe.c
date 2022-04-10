#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

struct fifo {
  struct spinlock lock;
  uint64 fa;      // pipe file address
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    kfree((char*)pi);
  } else
    release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();

  acquire(&pi->lock);
  while(i < n){
    if(pi->readopen == 0 || pr->killed){
      release(&pi->lock);
      return -1;
    }
    if(pi->nwrite == pi->nread + PIPESIZE){ //DOC: pipewrite-full
      wakeup(&pi->nread);
      sleep(&pi->nwrite, &pi->lock);
    } else {
      char ch;
      if(copyin(pr->pagetable, &ch, addr + i, 1) == -1)
        break;
      pi->data[pi->nwrite++ % PIPESIZE] = ch;
      i++;
    }
  }
  wakeup(&pi->nread);
  release(&pi->lock);

  return i;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int i;
  struct proc *pr = myproc();
  char ch;

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(pr->killed){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
  }
  for(i = 0; i < n; i++){  //DOC: piperead-copy
    if(pi->nread == pi->nwrite)
      break;
    ch = pi->data[pi->nread++ % PIPESIZE];
    if(copyout(pr->pagetable, addr + i, &ch, 1) == -1)
      break;
  }
  wakeup(&pi->nwrite);  //DOC: piperead-wakeup
  release(&pi->lock);
  return i;
}

struct fifo*
fifoalloc()
{
  void* fa;
  struct fifo *fi;
  
  fi = 0;
  fa = 0;
  if((fa = kalloc()) == 0 || (fi = (struct fifo*)kalloc()) == 0){
    if(fa)
      kfree(fa);
    return 0;
  }

  fi->fa = (uint64)fa;
  fi->readopen = 0;
  fi->writeopen = 0;
  fi->nwrite = 0;
  fi->nread = 0;
  initlock(&fi->lock, "fifo");

  return fi;
}

void
fifoclose(struct fifo *fi, int writable)
{
  acquire(&fi->lock);
  if(writable){            // close write
    fi->writeopen = 0;
    wakeup(&fi->nread);
  } else {                 // close read
    fi->readopen = 0;
    wakeup(&fi->nwrite);
  }
  
  if(fi->readopen == 0 && fi->writeopen == 0){
    release(&fi->lock);
    kfree((char*)fi);
  } else
    release(&fi->lock);
}

int
fifowrite(struct fifo *fi, uint64 src, int n)
{
  uint i;
  struct proc *pr;

  i = 0;
  pr = myproc();

  acquire(&fi->lock);
  fi->writeopen = 1;
  wakeup(&fi->nread);
  //printf("%p readopen: %d writeopen: %d\n", fi, fi->readopen, fi->writeopen);
  if(fi->readopen == 0){
    //printf("1write sleep\n");
    sleep(&fi->nwrite, &fi->lock);
    //printf("1write wake up\n");
  }
  while(i < n){
    if(pr->killed){
      release(&fi->lock);
      return -1;
    }
    if(fi->nwrite == fi->nread + PGSIZE){ // fifowrite-full
      wakeup(&fi->nread);
      //printf("2write sleep\n");
      sleep(&fi->nwrite, &fi->lock);
      //printf("2write wake up\n");
    } else {
      uint len = PGSIZE + fi->nread - fi->nwrite;
      if(n < len)                        // ensure no overwrite
        len = n;
      char* dst = (char*)(fi->fa + fi->nwrite % PGSIZE);
      if(copyin(pr->pagetable, dst, src + i, len) == -1)
        break;
      fi->nwrite += len;
      i += len;
    }
  }
  
  wakeup(&fi->nread);
  release(&fi->lock);
  
  return i;
}

int
fiforead(struct fifo *fi, uint64 dst, int n)
{
  uint i;
  struct proc *pr;

  i = 0;
  pr = myproc();
  
  acquire(&fi->lock);
  fi->readopen = 1;
  wakeup(&fi->nwrite);
  //printf("%p readopen: %d writeopen: %d\n", fi, fi->readopen, fi->writeopen);
  if(fi->writeopen == 0){
    //printf("1read sleep\n");
    sleep(&fi->nread, &fi->lock);
    //printf("1read wake up\n");
  }
  while(fi->nread == fi->nwrite && fi->writeopen){  // fifo-empty
    if(pr->killed){
      release(&fi->lock);
      return -1;
    }
    //printf("2read sleep\n");
    sleep(&fi->nread, &fi->lock);     // fiforead-sleep
    //printf("2read wake up\n");
  }
  while(i < n){                       // fiforead-copy
    if(fi->nread == fi->nwrite)
      break;
    uint len = fi->nwrite - fi->nread;
    if(n < len)                       // ensure no overread
      len = n;
    char* src = (char*)(fi->fa + fi->nread % PGSIZE);
    if(copyout(pr->pagetable, dst + i, src, len) == -1)
      break;
    fi->nread += len;
    i += len;
  }
  wakeup(&fi->nwrite);  // piperead-wakeup
  release(&fi->lock);
  return i;
}
