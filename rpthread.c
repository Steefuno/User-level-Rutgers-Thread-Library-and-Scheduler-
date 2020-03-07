// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
struct itimerval schedulerTimer; //Timer that interrupts threads if threads are too slow
rpthread_listItem_t* currentItem = NULL;

/* 0 if not specified, probably thread ended
 * 1 if timer interrupted
 * 2 if mutex blocked
 */
int proceedState = 0; //How the previous thread has closed
rpthread_mutex_t* currentMutex = NULL; //When proceedState is PROCEEDBYMUTEX, store the mutex that is blocking
tcb* joinToTCB = NULL; //When proceedState is PROCEEDBYJOIN, store the tcb to be joined

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// Initialize scheduler if NULL
	if (schedulerContext == NULL) initScheduler();

	// Create Thread Control Block
	tcb* threadBlock = (tcb*)malloc(sizeof(tcb));
	(*threadBlock).status = SCHEDULED;
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
	rpthread_listItem_t* listItem = insertIntoScheduler(threadBlock);
	thread = (rpthread_t*) listItem; //thread will reference the listItem
	
	// not sure what this should return
	return 1;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	// Change tcb state from RUNNING to READY
	// Save context of this thread to its thread control block
	// switch from thread context to scheduler context

	proceedState = PROCEEDBYYIELD;
	swapcontext((*((*currentItem).block)).context, schedulerContext);
	return 0;
};

/* keeps the tcb, but deallocates the stack of the select listItem */
// To be used after pthread_exit in scheduler
void deallocContext(rpthread_listItem_* listItem) {
	// Deallocate the stack
	free((*((*listItem).block)).stack);

	// Dereferences the stack
	(*((*listItem).block)).stack = NULL;

	return;
}

/* deallocates the block of the select listItem */
// To be used when joined
void deallocTCB(rpthread_listItem_t* listItem) {
	//Frees block
	free((*listItem).block);

	//Frees listItem
	free(listItem);

	return;
}

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Mark as ended to be joined
	proceedState = PROCEEDBYFINISH;

	// Set value_ptr to something
	*value_ptr = 1;

	// Setcontext to scheduler to handle deallocate context
	setcontext(schedulerContext);
};


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	// Wait for a specific thread to terminate
	// De-allocate any dynamic memory created by the joining thread

	// Get listItem for thread
	rpthread_listItem_t* toJoinItem = (rpthread_listItem_t*)thread;
	tcb* block = (*toJoinItem).block;

	// Wait until toJoinItem's block's status is ENDED
	while ((*block).status != ENDED) {
		proceedState = PROCEEDBYJOIN;
		// Swap to scheduler
		swapcontext((*((*currentItem).block)).context, schedulerContext);
	}

	return 1;
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

/* Handle current if scheduler is triggered by interrupt */
// Add back to queue and adjust priority
void stcfProceedByInterrupt() {
	// Get time since current started
	struct timeval timeEnd;
	gettimeofday(&timeEnd, 0);

	// Increment current's time
	(*(*currentItem).block).priority = (
		(*(*currentItem).block).priority
		+ (double) (
			((timeEnd.tv_sec-prevTick.tv_sec)*1000000)
			+ (timeEnd.tv_usec-prevTick.tv_usec)
		)
	);

	// Set status to ready
	(*(*currentItem).block).status = SCHEDULED;

	// Reposition current in queue
	// If current is the only thing in the list
	if (rpthread_threadList == NULL
		//Or if current has a lower priority than first in list
		|| (*(*rpthread_threadList).block).priority > (*(*currentItem).block).priority) {
		//Insert current as first in list
		(*currentItem).next = rpthread_threadList;
		rpthead_threadList = currentItem;
	} else { 
		// Iterate through queue until found an item 
		rpthread_listItem_t* listItem = rpthread_threadList;

		//While not at end of queue
		while ((*listItem).next != NULL
			//And if next item has lower time than current
			&& (*(*((*listItem).next)).block).priority < (*(*currentItem).block).priority) {
			listItem = (*listItem).next;
		}

		// Either at end of list or next item is bigger than current
		// So set current in between
		(*currentItem).next = (*listItem).next;
		(*listItem).next = currentItem;
	}
	return;
}

/* Preemptive SJF (STCF) scheduling algorithm */
void sched_stcf() {
	/* Handle current tcb */

	if (proceedState == PROCEEDBYTIMER) { // If interrupted by timer
		stcfProceedByInterrupt();
		(*(*currentItem).block).status = SCHEDULED;
	} else if (proceedState == PROCEEDBYYIELD) { // If interrupted by yielding
		stcfProceedByInterrupt();
		(*(*currentItem).block).status = READY;
	} else if (proceedState == PROCEEDBYMUTEX) { //If blocked by mutex
		stcfProceedByInterrupt();
		(*(*currentItem).block).status = BLOCKED;
	} else if (proceedState == PROCEEDBYJOIN) { //If waiting to join on a thread
		stcfProceedByInterrupt();
		(*(*currentItem).block).status = BLOCKED;
	} else { // If current has ended or exited, proceedState == PROCEEDBYFINISH
		deallocContext();
		(*(*currentItem).block).status = ENDED;
	}

	/* Start handling new tcb */

	//Prepare proceedState 
	proceedState = ENDED;

	// Store time of when next thread start
	gettimeofday(&prevTick, 0);

	// Start/Reset Timer
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	// Assume queue has atleast one item, should be main function atleast
	// Pop from queue
	currentItem = rpthread_threadList;
	rpthread_threadList = (*rpthread_threadList).next;
	// Ignore checking state, unneccessary

	// Set to Running state
	(*(*currentItem).block).status = RUNNING;

	// SetContext new item
	setcontext(
		&(
			((*currentItem).block).context
		)
	);
}

/* Preemptive MLFQ scheduling algorithm */
void sched_mlfq() {
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
	proceedState = PROCEEDBYTIMER; //Marks that scheduler was triggered by timer
	swapcontext(&((*(*currentItem).block).context), schedulerContext);
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
	insertIntoSTCFQueue(listItem, &rpthread_threadList);
	return;
}

/* Append into rpthread_MLFQ[0] */
void insertIntoMLFQ(rpthread_listItem_t* listItem, int selectLevel) {
	(*listItem).next = NULL;

	//If not setup yet, create levels
	if (rpthread_MLFQ == NULL) {
		rpthread_MLFQ = (rpthread_listItem_t**) malloc(sizeof(rpthread_listItem_t)*MLFQLEVELS);

		//Set all levels to NULL
		int i=0;
		while (i < MLFQLevels) {
			rpthread_MLFQ[i++] = NULL;
		}
	}

	//Insert into MLFQ queue at select level
	insertIntoSTCFQueue(listItem, &(rpthread_MLFQ[selectLevel]));
	return;
}

/* Add into a queue */
//Second argument is a pointer to a queuePtr
void insertIntoSTCFQueue(rpthread_listItem_t* listItem, rpthread_listItem** queuePtr) {
	(*listItem).next = NULL;

	//If queue DNE
	if (*queuePtr == NULL) {
		(*queuePtr) = listItem;
		return;
	}

	//if first in queue has been in use longer than listItem
	if ((*((*(*queuePtr)).block)).priority > (*((*listItem).block)).priority) {
		//Set listItem as first in queue
		(*listItem).next = (*queuePtr);
		(*queuePtr) = listItem;
		return;
	}

	rpthread_listItem* item = (*queuePtr);
	//Iterate until next has been in use longer than listItem
	while (*((*((*item).next)).block).priority > (*((*listItem).block)).priority) {
		item = (*item).next;
	}

	//Insert listItem
	(*listItem).next = (*item).next;
	(*item).next = listItem;
	return;
}

// Feel free to add any other functions you need
