// File:	rpthread_t.h

// List all group member's name:
// username of iLab:
// iLab Server:

#ifndef RTHREAD_T_H
#define RTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_RTHREAD macro */
#define USE_RTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <ucontext.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint rpthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread status
	int status;
	// thread context
	ucontext_t context;
	// thread stack
	void* stack;
	// thread priority
	double priority; //Ignore in MLFQ, used to show length in PSJF
	// 

	// ...
} tcb; 

/* Linked List for tcb queues */
typedef struct rpthread_listItem_t {
	tcb* block; //tcp
	struct rpthread_listItem_t* next; //next tcp
} rpthread_listItem_t;

/* mutex struct definition */
typedef struct rpthread_mutex_t {
	// thread that locked this mutex
	// NULL if unlocked
	rpthread_listItem_t* thread;

	// linked list of threads blocked
	rpthread_listItem_t* blocked;

	// ...
} rpthread_mutex_t;

/* Context to swap to to use scheduler */
ucontext_t* schedulerContext;

/* TCB for main code */
tcb* mainTCB;

rpthread_listItem_t* rpthread_threadList; //Used in PSJF
rpthread_listItem_t** rpthread_MLFQ; //rpthread_MLFQ[0] is top level, used in MLFQ scheduling

struct timeval prevTick; //Stores when the most recent thread started

/* Function Declarations: */

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int rpthread_yield();

/* keep tcb, but deallocated the current context and stack */
void deallocContext(rpthread_listItem_t* listItem);

/* deallocate the tcb */
void deallocTCB(rpthread_listItem_t* listItem);

/* terminate a thread */
void rpthread_exit(void *value_ptr);

/* wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr);

/* initial the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex);

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex);

/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex);

/* helper function to convert void* (*function)(void*) to void* (*function)(void*) */
void makecontextHelper();

/* scheduler */
static void schedule();

/* handle swapping out of a tcb if interrupted sctf */
void stcfProceedByInterrupt();

/* scheduler if stcf mode*/
void sched_stcf();

/* scheduler if mlfq mode */
void sched_mlfq();

/* initialize scheduler */
void initScheduler();

/* function that triggers on timer to swap to scheduler context */
void swapToScheduler(int sigNum);

/* adds a thread control block into the scheduling queue as READY */
rpthread_listItem_t* insertIntoScheduler(tcb* threadBlock);

/* add into STCF scheduling queue */
void insertIntoSTCF(rpthread_listItem_t* listItem);

/* add into MLFQ scheduling queue at the select level */
void insertIntoMLFQ(rpthread_listItem_t* listItem, int selectLevel);

/* add into Yielding queue, used in both schedulers */
//queuePtr is a pointer to the pointer that points at the first item in the linked list
void insertIntoSTCFQueue(rpthread_listItem_t* listItem, rpthread_listItem_t** queuePtr);

/* moves threads blocked by mutex back into queue */
void restoreBlockedToQueue(rpthread_mutex_t* mutex);

#ifdef USE_RTHREAD
#define pthread_t rpthread_t
#define pthread_mutex_t rpthread_mutex_t
#define pthread_create rpthread_create
#define pthread_exit rpthread_exit
#define pthread_join rpthread_join
#define pthread_mutex_init rpthread_mutex_init
#define pthread_mutex_lock rpthread_mutex_lock
#define pthread_mutex_unlock rpthread_mutex_unlock
#define pthread_mutex_destroy rpthread_mutex_destroy

//TCB States
#define RUNNING 0 //Current TCB
#define SCHEDULED 1 //Enqueued
#define READY 2 //Enqueued, but manually yielded
#define BLOCKED 3 //Blocked by mutex
#define ENDED 4 //Finished function and waiting to be joined

//Proceed States
//Different states to proceed into schedule() function
#define PROCEEDBYFINISH 0 //Finished thread function
#define PROCEEDBYTIMER 1 //Timer interrupted
#define PROCEEDBYMUTEX 2 //Attempt to lock mutex, but already locked
#define PROCEEDBYJOIN 3 //Yielding for another thread to finish
#define PROCEEDBYYIELD 4 //Yielding to give others omre schedule time

//Constants
//Probably need to change Stack Size
#define THREADSTACKSIZE 8192 //size of stack for a created thread
#define SCHEDULERSTACKSIZE 2048 
#define TICKSEC 0
#define TICKUSEC 2500 //picoseconds of time for timer to end, 1000 is 1ms
#define MLFQLEVELS 8 //number of levels in mlfq

#endif

#endif
