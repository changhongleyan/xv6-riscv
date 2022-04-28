#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"

#define NSHM         16  // maximum number of shared memory
#define NSHMPAGE     16  // maximum page number of a shared memory
#define MAXSHMSIZE   NSHMPAGE*PGSIZE // maximum size of a shared memory
struct shm{
  int ref;
  uint64 pas[NSHMPAGE];  // physical address array for kalloc()
  struct spinlock lock;
};

struct shm shms[NSHM];

void
shminit()
{
  for(int i = 0; i < NSHM; ++i){
    initlock(&shms[i].lock, "shm");
  }
}

uint64
shmget(int key, int size, int shmflg)
{ 
  struct proc* p = myproc();
  size = PGROUNDUP(size);
  if(p->sz > MAXVA - size || size > MAXSHMSIZE)
    return 0;
  
  int id = key % NSHM;
  struct shm* s = &shms[id];
  acquire(&s->lock);
  if(s->ref == 0 && (shmflg & IPC_CREATE) == 0){
    release(&s->lock);
    return 0;
  }
  ++s->ref;
  release(&s->lock);
  return mmap(p->sz, size, 0, 0, 0, 0, id);
}

uint64
shmpa_get(int id, int nth)
{ 
  if(nth >= NSHMPAGE)
    return 0;
  struct shm* s = &shms[id];
  uint64* pap = &s->pas[nth]; // physical address pointer
  acquire(&s->lock);
  if(*pap == 0){
    *pap = (uint64)kalloc();
  }
  release(&s->lock);
  return *pap;
}

// return shared memory's reference count
int
shmclose(int id)
{
  struct shm* s = &shms[id];
  acquire(&s->lock);
  if(s->ref < 1)
    panic("shmclose");
  if(--s->ref == 0){
    memset(s->pas, 0, sizeof(s->pas));
  }
  int ref = s->ref;
  release(&s->lock);
  return ref;
}
