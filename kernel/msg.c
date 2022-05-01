#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"

#define NMSQ         8     // maximum number of message queue
#define MAXMSGSIZE   1024

struct msg_msg{
  int used;
  struct msg_msg* pre;
  struct msg_msg* next;
  int type;
  char data[MAXMSGSIZE];
  int length;
};

// 因xv6未实现内存池，只能动态分配PGSIZE大小的内存，故预先分配好固定数量msg
#define NMSG         64
struct msg_msg msg_msgs[NMSG];
struct spinlock getmsg;

struct msg_q{
  int msg_num;
  int ref;
  struct msg_msg* first_msg;
  struct msg_msg* last_msg;
  // 暂不实现阻塞队列的功能，理由同上
  // struct blk_q q_receivers;
  // struct blk_q q_senders;
  struct spinlock lock;
};

struct msg_q msg_qs[NMSQ];

void
msginit()
{
  for(int i = 0; i < NMSQ; ++i){
    initlock(&msg_qs[i].lock, "msq");
  }
  initlock(&getmsg, "getmsg");
}


int
msgget(int key, int msgflg)
{
  // proc->msg_qid[i] = 0 means unused，so id != 0
  int id = key % (NMSQ-1) + 1;
  struct proc* p = myproc();
  struct msg_q* msq = &msg_qs[id];

  // exist
  for(int i = 0; i < PMSQSIZE; ++i){
    if(p->msg_qid[i] == id)
      return id;
  }

  // create
  acquire(&msq->lock);
  if(msq->ref == 0 && (msgflg & IPC_CREATE) == 0){
    release(&msq->lock);
    return -1;
  }
  for(int i = 0; i < PMSQSIZE; ++i){
    if(p->msg_qid[i] == 0){
      p->msg_qid[i] = id;
      ++msq->ref;
      release(&msq->lock);
      return id;
    }
  }
  release(&msq->lock);
  printf("msgget: max number of message queue\n");
  return -1;
}

int
msgdup(int id)
{
  if(id <= 0 || id >= NMSQ){
    printf("msgdup: id error\n");
    return -1;
  }
  struct msg_q* msq = &msg_qs[id];
  acquire(&msq->lock);
  if(msq->ref < 1)
    panic("msgdup");
  ++msq->ref;
  release(&msq->lock);
  return id;
}

struct msg_msg*
msgalloc()
{
  for(int i = 0; i < NMSG; ++i){
    if(msg_msgs[i].used == 0){
      msg_msgs[i].used = 1;
      return &msg_msgs[i];
    }
  }
  return 0;
}

int
msgsnd(int id, uint64 va, int length)
{
  if(id <= 0 || id >= NMSQ){
    printf("msgsnd: id error\n");
    return -1;
  }
  if(length > MAXMSGSIZE){
    printf("msgsnd: exceed message max size\n");
    return -1;
  }
  
  struct proc* p = myproc();
  struct msg_q* msq = &msg_qs[id];
  // struct msg_msg* msg = (struct msg_msg*)kmalloc(sizeof(struct msg_msg));
  struct msg_msg* msg;
  acquire(&getmsg);
  while((msg = msgalloc()) == 0){
    sleep(msq, &getmsg);
  }
  release(&getmsg);

  if(copyin(p->pagetable, (char*)&msg->type, va, sizeof(msg->type))){
    msg->used = 0;
    return -1;
  }
  va += sizeof(msg->type);
  if(copyin(p->pagetable, (char*)&msg->data, va, length)){
    msg->used = 0;
    return -1;
  }
  msg->length = length;

  acquire(&msq->lock);
  if(msq->last_msg){
    msq->last_msg->next = msg;
    msg->pre = msq->last_msg;
    msq->last_msg = msg;
  } else {
    msq->first_msg = msg;
    msq->last_msg = msg;
  }
  ++msq->msg_num;
  release(&msq->lock);

  return 0;
}

int
msgrcv(int id, uint64 va, int size, int type)
{
  if(id <= 0 || id >= NMSQ){
    printf("msgrcv: id error\n");
    return -1;
  }
  struct proc* p = myproc();
  struct msg_q* msq = &msg_qs[id];
  
  // 没实现阻塞队列，默认flag == MSG_NOWAIT
  if(msq->first_msg == 0){
    return -1;
  }
  
  acquire(&msq->lock);
  struct msg_msg* msg = msq->first_msg;
  while(msg){
    if(msg->type == type && msg->length <= size)
      break;
    msg = msg->next;
  }
  if(msg == 0){
    release(&msq->lock);
    return -1;
  }

  if(msg == msq->first_msg){
    msq->first_msg = msg->next;
  }
  if(msg == msq->last_msg){
    msq->last_msg = msg->pre;
  }

  if(msg->pre){
    msg->pre->next = msg->next;
  }
  if(msg->next){
    msg->next->pre = msg->pre;
  }
  --msq->msg_num;
  release(&msq->lock);
  
  if(copyout(p->pagetable, va, (char*)&msg->type, sizeof(msg->type)))
    return -1;
  va += sizeof(msg->type);
  if(copyout(p->pagetable, va, (char*)&msg->data, msg->length))
    return -1;
  int msg_length = msg->length;
  memset(msg, 0, sizeof(struct msg_msg));
  wakeup1p(msq);

  return msg_length;
}

int
msgclose(int id)
{
  if(id <= 0 || id >= NMSQ){
    printf("msgclose: id error\n");
    return -1;
  }
  struct msg_q* msq = &msg_qs[id];
  acquire(&msq->lock);
  if(--msq->ref == 0 && msq->msg_num){
    struct msg_msg* p;
    while(msq->msg_num){
      p = msq->first_msg;
      msq->first_msg = msq->first_msg->next;
      memset(p, 0, sizeof(struct msg_msg));
      --msq->msg_num;
    }
  }
  int ref = msq->ref;
  release(&msq->lock);
  return ref;
}