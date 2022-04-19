#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

#define NMSQ         16  // maximum number of message queue
struct msg{
  struct msg* next;
  uint type;
  uint64 mpa;           // message physical address
  uint length;          
};

struct msq{
    struct msg* first;
    struct msg* last;
    int msg_num;
    int ref;
    struct wait_queue* q_receivers;
    struct wait_queue* q_senders;
    struct spinlock lock;
};

struct msq msqs[NMSQ];

void
msqsinit()
{
    char lkname[10] = "msqlock_";
    for(int i = 0; i < NMSQ; ++i){
        lkname[8] = 'a' + i;
        initlock(&msqs[i].lock, lkname);
    }
}

int
msgget(int key)
{
  int id = key % NMSQ;
  struct msq* msq = &msqs[id];
  acquire(&msq->lock);
  if(msq->first == 0){
    
  }
  return id;
}
