#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

#define NSEM 10

typedef struct semaphore {
    struct spinlock lock;
    int value;
    int count;
    int used;
}sem_t;

sem_t sems[NSEM];

void
seminit()
{
    for(int i = 0; i < NSEM; ++i){
        initlock(&sems[i].lock, "sem");
    }
}

int
semget(int initval)
{
    for(int i = 0; i < NSEM; ++i){
        acquire(&sems[i].lock);
        if(!sems[i].used){
            sems[i].used = 1;
            sems[i].value = initval;
            sems[i].count = initval;
            release(&sems[i].lock);
            return i;
        }
        release(&sems[i].lock);
    }
    return -1;
}

int
sem_p(int id)
{
    sem_t* sem = &sems[id];
    acquire(&sem->lock);
    if(!sem->used){
        release(&sem->lock);
        return -1;
    }

    --sem->count;
    //printf("%d p: %d\n", id, sem->count);
    if(sem->count < 0){
        sleep(sem, &sem->lock);
    }
    release(&sem->lock);
    return 0;
}

int
sem_v(int id)
{
    sem_t* sem = &sems[id];
    acquire(&sem->lock);
    if(!sem->used){
        release(&sem->lock);
        return -1;
    }

    ++sem->count;
    //printf("%d v: %d\n", id, sem->count);
    if(sem->count <= 0){
        wakeup1p(sem);
    }
    release(&sem->lock);
    return 0;
}

int
semclose(int id)
{
    sem_t* sem = &sems[id];

    acquire(&sem->lock);
    if(sem->used && sem->count == sem->value) {
        sem->used = 0;
        release(&sem->lock);
        return 0;
    }
    release(&sem->lock);
    return -1;
}