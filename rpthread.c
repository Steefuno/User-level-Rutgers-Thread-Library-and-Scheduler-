// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
int nextThreadId = 1;
struct itimerval schedulerTimer; //Timer that interrupts threads if threads are too slow
rethread_listItem_t* currentItem = NULL;

/* 0 if not specified, probably thread ended
 * 1 if timer interrupted
 * 2 if mutex blocked
 */
int proceedState = 0; //How the previous thread has closed


/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// Initialize scheduler if NULL
	if (schedulerContext == NULL) initScheduler();

	// Create Thread Control Block
	tcb* threadBlock = (tcb*)malloc(sizeof(tcb));
	(*threadBlock).id = nextThreadId++;
	(*threadBlock).status = READY;
	(*threadBlock).priority = 0; //top priority

	//Setup context
	ucontext_t* context = &((threadBlock*).context);
	void* contextStack = malloc(THREADSTACKSIZE);
	(*threadBlock).stack = contextStack;

	//Setup stack and uc_link (return context) to point to scheduler
	getcontext(context);
	(*context).uc_stack.ss_sp = contextStack;
	(*context).uc_stack.ss_size = THREADSTACKSIZE;
	(*context).uc_link = schedulerContext;
	makecontext(*context, function)

	// add new tcb into queue
	insertIntoScheduler(threadBlock);
	
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

	// schedule policy
	#ifndef MLFQ
		// Choose STCF
		sched_stcf();
	#else 
		// Choose MLFQ
		sched_mlfq();
	#endif
}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// If triggered by timer
	if (proceedState == PROCEEDBYTIMER) {
		// Get time since current started
		struct timeval timeEnd;
		gettimeofday(&timeEnd, 0);

		// Increment current's time
		(*current).priority = (
			(*current).priority
			+ (double) (
				((timeEnd.tv_sec-prevTick.tv_sec)*1000000)
				+ (timeEnd.tv_usec-prevTick.tv_usec)
			)
		);

		// Set status to ready
		(*current).status = READY;

		// Reposition current in queue
		
	} else if (proceedState == PROCEEDBYMUTEX) { //If proceed by mutex
		//Set status to blocked and 
	} else { // Current thread has ended
		// Check jointhreads to trigger
		

		// Remove from queue
		
	}
	proceedState = 0;

	// Store time of when next thread start
	gettimeofday(&prevTick, 0);

	// Start Timer
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	// Get first item in queue
	rpthread_listItem_t* item = rpthread_threadList;
	

	// SetContext to first in queue
	setcontext(
		&(
			(
				*(*threadList).block
			).context
		)
	);
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// If 
}

/* Setup scheduler context */
void initScheduler () {
	if (schedulerContext != NULL) return;

	//Create and setup scheduler context
	schedulerContext = (ucontext_t*)malloc(sizeof(ucontext_t));

	void* schedulerStack = malloc(SCHEDULERSTACKSIZE);

	getcontext(schedulerContext);
	(*schedulerContext).uc_stack.ss_sp = schedulerStack;
	(*schedulerContext).uc_stack.ss_size = SCHEDULERSTACKSIZE;
	makecontext(*schedulerContext, schedule);

	//Sets tcb to represent main
	mainTCB = (tcb*)malloc(sizeof(tcb));
	(*mainTCB).id = 0;
	(*mainTCB).status = SCHEDULED;
	(*mainTCB).priority = 0;

	ucontext_t* context = &((*mainTCB).context);
	getcontext(context);
	(*mainTCB).stack = (*context).uc_stack.ss_sp;
	
	//Insert mainTCB into queue
	rpthread_listItem_t* mainListItem = insertIntoScheduler(mainTCB);
	//Set mainTCB as current
	currentItem = mainListItem;

	//Setup sigaction to do timerHandler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &swapToScheduler;
	//Setup sigaction to trigger on SIGVTALRM
	sigaction(SIGVTALRM, &sa, NULL);

	//Trigger SIGVTALRM every TICKSEC + TICKUSEC
	schedulerTimer.it_value.tv_sec = TICKSEC;
	schedulerTimer.it_value.tv_usec = TICKUSEC;

	//Start timer
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	//Return to adding thread
	return;
}

void swapToScheduler(int sigNum) {
	//Proceed to scheduler
	proceedState = 1; //Marks that scheduler was triggered by timer
	swapcontext(&((*currentItem).context), schedulerContext);
	//Stores current context into currentItem and swaps to scheduler
}

/* adds a tcb into the scheduling queue as READY */
/* returns the listItem of tcb in queue */
rpthread_listItem_t* insertIntoScheduler(tcb* threadBlock) {
	//Create listItem
	rpthread_listItem* listItem = (rpthread_listItem*)malloc(sizeof(rpthread_listItem));

	//Set tcb
	(*listItem).block = threadBlock;
	(*listItem).next = NULL;

	//Check type of scheduler
	//Not sure, but I think this is how to check
	#ifndef MLFQ //If STCF
		insertIntoSTCF(listItem);
	#else //If MLFQ
		insertIntoMLFQ(listItem);
	#endif

	return listItem;
}

/* Insert into rpthread_threadList */
void insertIntoSTCF(rpthread_listItem_t* listItem) {
	//Set listItem as new beginning of list and set current as next
	(*listItem).next = rpthread_threadList;
	rpthread_threadList = listItem;

	return;
}

/* Append into rpthread_MLFQ[0] */
void insertIntoMLFQ(rpthread_listItem_t* listItem) {
	//If not setup yet, create levels
	if (rpthread_MLFQ == NULL) {
		rpthread_MLFQ = (rpthread_listItem_t**) malloc(sizeof(rpthread_listItem_t)*MLFQLEVELS);
		//Set item (main thread) as first item
		rpthread_MLFQ[0] = listItem;
		return;
	}

	//Iterate through level 0 and append Item to end
	rpthread_listItem_t* itemPtr = rpthread_MLFQ[0];
	while ((*itemPtr).next != NULL) {
		itemPtr = (*itemPtr).next;
	}
	(*itemPtr).next = listItem;
	return;
}

// Feel free to add any other functions you need
