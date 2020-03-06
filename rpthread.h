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
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint rpthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread id
	int id;
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

//Linked List
typedef struct rpthread_listItem_t {
	tcb* block; //tcp
	struct rpthread_listItem_t* next; //next tcp
} rpthread_listItem_t;

/* mutex struct definition */
typedef struct rpthread_mutex_t {
	// thread that locked this mutex
	// NULL if unlocked
	tcb* callerThread;

	// list of threads blocked by this mutex
	rpthread_listItem_t* blocks;

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

/* scheduler */
static void schedule();

/* initialize scheduler */
void initScheduler();

/* function that triggers on timer to swap to scheduler context */
void swapToScheduler(int sigNum);

/* adds a thread control block into the scheduling queue as READY */
rpthread_listItem_t* insertIntoScheduler(tcb* threadBlock);

/* add into STCF scheduling queue */
void insertIntoSTCF(rpthread_listItem_t* listItem);

/* add into MLFQ scheduling queue */
void insertIntoMLFQ(rpthread_listItem_t* listItem);

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

//Thread States
#define READY 1
#define SCHEDULED 2
#define BLOCKED 3
#define ENDED 4

//Proceed States
#define PROCEEDBYFINISH 0
#define PROCEEDBYTIMER 1
#define PROCEEDBYMUTEX 2
#define PROCEEDBYJOIN 3

//Constants
//Probably need to change Stack Size
#define THREADSTACKSIZE 8192
#define SCHEDULERSTACKSIZE 2048
#define TICKSEC 0
#define TICKUSEC 20000 //1000 is 1ms
#define MLFQLEVELS 8

#endif

#endif
