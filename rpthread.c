// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
int nextThreadId = 0;

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// Initialize scheduler if NULL
	if (schedulerContext == NULL) _initScheduler();

	// Create Thread Control Block
	tcb* threadBlock = (tcb*)malloc(sizeof(tcb));
	(*threadBlock).id = nextThreadId++;
	(*threadBlock).status = READY;

	//Setup context
	ucontext_t* context = &((threadBlock*).context);
	void* contextStack = malloc(THREADSTACKSIZE);

	getcontext(context);
	(*context).uc_stack.ss_sp = contextStack;
	(*context).uc_stack.ss_size = THREADSTACKSIZE;
	(*context).uc_link = schedulerContext;
	makecontext(*context, function)

	// after everything is all set, push this thread into scheduler
	
	
	// not sure what this should return
	return (*threadBlock).id;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// switch from thread context to scheduler context

	// YOUR CODE HERE
	return 0;
};

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	// Wait for a specific thread to terminate
	// De-allocate any dynamic memory created by the joining thread

	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	//Initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
	// use the built-in test-and-set atomic function to test the mutex
	// When the mutex is acquired successfully, enter the critical section
	// If acquiring mutex fails, push current thread into block list and 
	// context switch to the scheduler thread

	// YOUR CODE HERE
	return 0;
};

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init

	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE

	// schedule policy
	#ifndef MLFQ
	// Choose STCF
	#else 
	// Choose MLFQ
	#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Setup scheduler context */
void _initScheduler () {
	if (schedulerContext != NULL) return;

	schedulerContext = (ucontext_t*)malloc(sizeof(ucontext_t));

	void* schedulerStack = malloc(SCHEDULERSTACKSIZE);

	getcontext(schedulerContext);
	(*schedulerContext).uc_stack.ss_sp = schedulerStack;
	(*schedulerContext).uc_stack.ss_size = SCHEDULERSTACKSIZE;
	//(*context).uc_link = ?? //Not sure what to set to appear when scheduler somehow ends
	makecontext(*schedulerContext, schedule);
}

// Feel free to add any other functions you need

