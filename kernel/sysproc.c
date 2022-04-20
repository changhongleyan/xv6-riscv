#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  //if(growproc(n) < 0)
  //  return -1;
  myproc()->sz += n;
  if(n < 0){
    uvmdealloc(myproc()->pagetable, addr, addr+n);
  }
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_shmget(void)
{ 
  int key, size, shmflg;
  if(argint(0, &key) || argint(1, &size) || argint(2, &shmflg))
    return -1;
  return shmget(key, size, shmflg);
}

uint64
sys_msgget(void)
{
  int key, msgflg;
  if(argint(0, &key) || argint(1, &msgflg))
    return -1;
  return msgget(key, msgflg);
}

uint64
sys_msgsnd(void)
{
  int id, length;
  uint64 va;
  if(argint(0, &id) || argaddr(1, &va) || argint(2, &length))
    return -1;
  return msgsnd(id, va, length);
}

uint64
sys_msgrcv(void)
{
  int id, size, type;
  uint64 va;
  if(argint(0, &id) || argaddr(1, &va) || argint(2, &size) || argint(3, &type))
    return -1;
  return msgrcv(id, va, size, type);
}

uint64
sys_msgclose(void)
{
  int id;
  if(argint(0, &id))
    return -1;
  return msgclose(id);
}