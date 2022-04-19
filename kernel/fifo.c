#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"

struct fifo {
  struct spinlock lock;
  uint64 fa;      // pipe file address
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

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
  //printf("%p readopen: %d writeopen: %d\n", fi, fi->readopen, fi->writeopen);
  if(fi->readopen == 0){
    sleep(&fi->nwrite, &fi->lock);
  }
  while(i < n){
    if(fi->readopen == 0 || pr->killed){
      release(&fi->lock);
      return -1;
    }
    if(fi->nwrite == fi->nread + PGSIZE){ // fifowrite-full
      wakeup(&fi->nread);
      //printf("2write sleep\n");
      sleep(&fi->nwrite, &fi->lock);
      //printf("2write wake up\n");
    } else {
      uint len;
      if(fi->nwrite % PGSIZE >= fi->nread % PGSIZE)    // ensure no overwrite
        len = PGSIZE - fi->nwrite % PGSIZE;
      else
        len = fi->nread % PGSIZE - fi->nwrite % PGSIZE;
      if(n < len)                        
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
  //printf("%p readopen: %d writeopen: %d\n", fi, fi->readopen, fi->writeopen);
  if(fi->writeopen == 0){
    sleep(&fi->nread, &fi->lock);
  }
  while(fi->nread == fi->nwrite){  // fifo-empty
    if(pr->killed){
      release(&fi->lock);
      return -1;
    }
    wakeup(&fi->nwrite);
    sleep(&fi->nread, &fi->lock);
  }
  while(i < n){                       // fiforead-copy
    if(fi->nread == fi->nwrite)
      break;
    uint len;
    if(fi->nwrite % PGSIZE >= fi->nread % PGSIZE)    // ensure no overread
      len = fi->nwrite % PGSIZE - fi->nread % PGSIZE;
    else
      len = PGSIZE - fi->nread % PGSIZE;
    if(n < len)                       
      len = n;
    char* src = (char*)(fi->fa + fi->nread % PGSIZE);
    if(copyout(pr->pagetable, dst + i, src, len) == -1)
      break;
    fi->nread += len;
    i += len;
  }
  wakeup(&fi->nwrite);
  release(&fi->lock);
  return i;
}
