// File:	rpthread.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "rpthread.h"

// INITAILIZE ALL YOUR VARIABLES HERE
static struct itimerval schedulerTimer; //Timer's time that interrupts threads if threads are too slow
static struct itimerval pausedTimer; //Timer's time that is set to 0,0 pauses timer
static rpthread_listItem_t* currentItem = NULL;

/* 0 if not specified, probably thread ended
 * 1 if timer interrupted
 * 2 if mutex blocked
 */
static int proceedState = 0; //How the previous thread has closed
static rpthread_mutex_t* proceedMutex = NULL; //The mutex if proceedState is PROCEEDBYMUTEX
void *(*makeFunction)(void*) = NULL; //Pointer to function to use makecontext on
int currentLevel = 0;

int rpthread_n = 0;

rpthread_listItem_t* rpthread_threadList = NULL;

/* create a new thread */
int rpthread_create(rpthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// Initialize scheduler if NULL
	if (schedulerContext == NULL) initScheduler();

	// Create Thread Control Block
	tcb* threadBlock = (tcb*)malloc(sizeof(tcb));
	(*threadBlock).status = SCHEDULED;
	(*threadBlock).priority = 0; //top priority

	//Setup context
	ucontext_t* context = &((*threadBlock).context);
	void* contextStack = malloc(THREADSTACKSIZE);
	(*threadBlock).stack = contextStack;

	//Setup stack and uc_link (return context) to point to scheduler
	getcontext(context);
	(*context).uc_stack.ss_sp = contextStack;
	(*context).uc_stack.ss_size = THREADSTACKSIZE;
	(*context).uc_link = schedulerContext;

	makecontext(context, makecontextHelper, 2, function, arg); // does makecontext

	// add new tcb into queue
	rpthread_listItem_t* listItem = insertIntoScheduler(threadBlock);
	*thread = (rpthread_t)listItem; //thread will reference the listItem as a rpthread_t
//	printf("\tCreated Thread: %d\n", *thread);

	// not sure what this should return
	return 1;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	// Change tcb state from RUNNING to READY
	// Save context of this thread to its thread control block
	// switch from thread context to scheduler context

	proceedState = PROCEEDBYYIELD;
	swapcontext(
		&(
			(
				*((*currentItem).block)
			).context
		),
		schedulerContext
	);
	return 0;
};

/* keeps the tcb, but deallocates the stack of the select listItem */
// To be used after pthread_exit in scheduler
void deallocContext(rpthread_listItem_t* listItem) {
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
	// Pause timer
	setitimer(ITIMER_PROF, &pausedTimer, NULL);

	// Mark as ended to be joined
	proceedState = PROCEEDBYFINISH;

	// Setcontext to scheduler to handle deallocate context
	setcontext(schedulerContext);
};


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	// Wait for a specific thread to terminate
	// De-allocate any dynamic memory created by the joining thread

	// Pause timer
	setitimer(ITIMER_PROF, &pausedTimer, NULL);

	// Get listItem for thread
	rpthread_listItem_t* toJoinItem = (rpthread_listItem_t*)thread;
	tcb* block = (*toJoinItem).block;

	// Position to revert to when given time in scheduler
	getcontext(
		&(
			(
				*((*currentItem).block)
			).context
		)
	);

	// If still trying to join, go back to scheduler
	if ((*block).status != ENDED) {
		proceedState = PROCEEDBYJOIN;
		// go to scheduler
		setcontext(schedulerContext);
	}

//	printf("\tJoined %d\n", toJoinItem);

	// Deallocate the ended block
	deallocTCB(toJoinItem);

	// Continue current
	return 1;
};

/* initialize the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex
	// Ignore mutexattr

	// Set thread as NULL, mutex already allocated
	(*mutex).thread = NULL;

	// Continue current
	return 1;
};

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
	// use the built-in test-and-set atomic function to test the mutex
	// When the mutex is acquired successfully, enter the critical section
	// If acquiring mutex fails, push current thread into block list and 
	// context switch to the scheduler thread

	// If mutex is unavailable to lock
	if ((*mutex).thread != NULL) {
		// Pause timer
		setitimer(ITIMER_PROF, &pausedTimer, NULL);

		// If mutex is destroyed
		if ((*mutex).thread == (rpthread_listItem_t*)1) {
			// Error
			printf("\tCannot lock a destroyed muted\n");
			exit(0);
		}

		// if locked by current thread
		if ((*mutex).thread == currentItem) {
			// Should error
			printf("\tCannot lock if already locked by this thread\n");
			exit(0);
		}

		// If mutex is occupied
		proceedState = PROCEEDBYMUTEX;
		proceedMutex = mutex;

		// go to scheduler to be temp removed from queue
		swapcontext(&((*(*currentItem).block).context), schedulerContext);

		// on given back access, continue to lock
	}

	// Set mutex's thread to current
	(*mutex).thread = currentItem;

	// Continue current
	return 1;
};

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.
	if ((*mutex).thread == (rpthread_listItem_t*)1) {
		// Error
		printf("\tCannot unlock a destroyed muted\n");
		exit(0);
	}

	if ((*mutex).thread != currentItem) {
		// Should error
		printf("\tCannot unlock when locked by another thread %d %d\n", (*mutex).thread, currentItem);
		exit(0);
	}

	// Remove current from mutex
	(*mutex).thread = NULL;

	// Add all blocked back to queue
	restoreBlockedToQueue(mutex);

	// Continue current
	return 1;
};


/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init
	if ((*mutex).thread == (rpthread_listItem_t*)1) {
		// Error
		printf("\tCannot destroy a destroyed muted\n");
		exit(0);
	}

	if ((*mutex).thread != NULL) {
		// Should error
		printf("\tCannot destroy when locked\n");
		exit(0);
	}

	(*mutex).thread = (rpthread_listItem_t*)1;

	return 1;
};

/* helper function to convert void* (*function)(void*) to void* (*function)(void) and use makecontext */
void makecontextHelper(void (*selectFunction)(), void* selectArg) {
	selectFunction(selectArg);
}

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

/* adjust's the currentItem's priority by the change in time */
void stcfAdjustPriority() {
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
}

/* Handle current if scheduler is triggered by interrupt */
// Add back to queue and adjust priority
void stcfProceedByInterrupt() {
	// Increment current's time
	stcfAdjustPriority();

	// Set status to ready
	(*(*currentItem).block).status = SCHEDULED;

	// Reposition current in queue
	// If current is the only thing in the list
	if (rpthread_threadList == NULL
		//Or if current has a lower priority than first in list
		|| (*(*rpthread_threadList).block).priority > (*(*currentItem).block).priority) {
		//Insert current as first in list
		(*currentItem).next = rpthread_threadList;
		rpthread_threadList = currentItem;
	} else { 
		/* Iterate through queue */

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

/* Adds current to mutex's blocked list */
void stcfProceedByMutex() {
	// Increment current's time
	stcfAdjustPriority();

	rpthread_mutex_t* mutex = proceedMutex;

	// Make current the head of blocked and current head as next
	(*currentItem).next = (*mutex).blocked;
	(*mutex).blocked = currentItem;
	// Don't enqueue curentItem
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
		stcfProceedByMutex();
		(*(*currentItem).block).status = BLOCKED;
	} else if (proceedState == PROCEEDBYJOIN) { //If waiting to join on a thread
		stcfProceedByInterrupt();
		(*(*currentItem).block).status = BLOCKED;
	} else { // If current has ended or exited, proceedState == PROCEEDBYFINISH
		deallocContext(currentItem);
		(*(*currentItem).block).status = ENDED;
	}

	/* Start handling new tcb */

	//Prepare proceedState 
	proceedState = ENDED;

	// Store time of when next thread start
	gettimeofday(&prevTick, 0);

	// Start/Reset Timer
	setitimer(ITIMER_PROF, &schedulerTimer, NULL);

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
			(*
				((*currentItem).block)
			).context
		)
	);
}

/* Preemptive MLFQ scheduling algorithm */
//Note: currently executing thread is always stored at the front of the MLFQ linked list
void sched_mlfq() {
	//Pause Timer
	setitimer(ITIMER_PROF, &schedulerTimer, NULL);

	//set status, print info
	if (proceedState == PROCEEDBYTIMER) { // If interrupted by timer
		(*(*currentItem).block).status = SCHEDULED;
	} else if (proceedState == PROCEEDBYYIELD) { // If interrupted by yielding
		(*(*currentItem).block).status = READY;
	} else if (proceedState == PROCEEDBYMUTEX) { //If blocked by mutex
		(*(*currentItem).block).status = BLOCKED;
	} else if (proceedState == PROCEEDBYJOIN) { //If waiting to join on a thread
		(*(*currentItem).block).status = BLOCKED;
	} else { // If current has ended or exited, proceedState == PROCEEDBYFINISH
		printf("\tProceeding by finish\n");
	}

	
	//printf("begin sched_mlfq %d\n",currentItem);	
	//remove currentItem from front of queue, unless queue is empty (first thread to be entered into queue)
	if(rpthread_MLFQ[currentLevel] != NULL)
		rpthread_MLFQ[currentLevel] = rpthread_MLFQ[currentLevel]->next;

	//If full time slice used and not already at the lowest level, move down a level
	if(proceedState == PROCEEDBYTIMER && currentLevel != (MLFQLEVELS-1)){
		//currentLevel++;
	}
	
	//if not finished, add thread back to queue
	if(proceedState==PROCEEDBYFINISH){
		deallocContext(currentItem);
		currentItem->block->status = ENDED;
	}else{
		insertIntoMLFQ(currentItem, currentLevel);
	}

	//pick next thread to execute, store in currentItem
	int i=0;
	while(rpthread_MLFQ[i]==NULL)
		i++;

	//this should never happen because MLFQ should never be empty
	if(i==MLFQLEVELS){
		printf("\tERROR: MLFQ EMPTY\n");
		return;
	}

	currentItem = rpthread_MLFQ[i];
	
	proceedState = ENDED;

	//set new timer
	setitimer(ITIMER_PROF, &schedulerTimer, NULL);

	//printf("end sched_mlfq\n");
	//start executing new thread
	setcontext(&(currentItem->block->context));

	
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
	makecontext(schedulerContext, &schedule, 0);

	//Sets tcb to represent main
	mainTCB = (tcb*)malloc(sizeof(tcb));
	(*mainTCB).status = SCHEDULED;
	(*mainTCB).priority = 0;

	ucontext_t* context = &((*mainTCB).context);
	getcontext(context);

	(*mainTCB).stack = (*context).uc_stack.ss_sp;
	
	//Insert mainTCB into queue to create mainListItem
	rpthread_listItem_t* mainListItem = insertIntoScheduler(mainTCB);
	//Set mainTCB as current
	currentItem = mainListItem;

	//Set head as null since main, head, is in use
	rpthread_threadList = NULL;

	//Setup sigaction to do timerHandler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &swapToScheduler;
	//Setup sigaction to trigger on SIGPROF
	sigaction(SIGPROF, &sa, NULL);

	//Trigger SIGPROF every TICKSEC + TICKUSEC
	schedulerTimer.it_value.tv_sec = TICKSEC;
	schedulerTimer.it_value.tv_usec = TICKUSEC;

	//itimerval to be used when pausing timer
	pausedTimer.it_value.tv_sec = 0;
	pausedTimer.it_value.tv_sec = 0;

	//Start timer
	setitimer(ITIMER_PROF, &schedulerTimer, NULL);

//	printf("\tScheduler Initialized, MainTCB is: %d, Head is: %d\n", mainListItem, rpthread_threadList);

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
	rpthread_listItem_t* listItem = (rpthread_listItem_t*)malloc(sizeof(rpthread_listItem_t));

	//Set tcb
	(*listItem).block = threadBlock;
	(*listItem).next = NULL;

	//Check type of scheduler
	//Not sure, but I think this is how to check
	#ifndef MLFQ //If STCF
		insertIntoSTCF(listItem);
	#else //If MLFQ
		insertIntoMLFQ(listItem, 0);
	#endif

	return listItem;
}

/* Insert into rpthread_threadList */
void insertIntoSTCF(rpthread_listItem_t* listItem) {
	insertIntoSTCFQueue(listItem, &rpthread_threadList);
	return;
}

/* Append into rpthread_MLFQ[0] */
void insertIntoMLFQ(rpthread_listItem_t* listItem,int selectLevel) {
	(*listItem).next = NULL;

	//If not setup yet, create levels
	if (rpthread_MLFQ == NULL) {
		rpthread_MLFQ = (rpthread_listItem_t**) malloc(sizeof(rpthread_listItem_t)*MLFQLEVELS);

		//Set all levels to NULL
		int i=0;
		while (i < MLFQLEVELS) {
			rpthread_MLFQ[i++] = NULL;
		}
	}

	//Insert into MLFQ queue at end of select level
	rpthread_listItem_t* ptr = rpthread_MLFQ[selectLevel];
	if(ptr==NULL){
		rpthread_MLFQ[selectLevel] = listItem;
	}else{

	 while(ptr!=NULL){
		if(ptr->next == NULL){
			ptr->next = listItem;
			break;
		}
		ptr = ptr->next;
	}
	
	}

	return;
}

/* Add into a queue */
//Second argument is a pointer to a queuePtr
void insertIntoSTCFQueue(rpthread_listItem_t* listItem, rpthread_listItem_t** queuePtr) {
	(*listItem).next = NULL;

	//If queue DNE
	if (*queuePtr == NULL) {
		(*queuePtr) = listItem;
		return;
	}

	//if listItem has a lower priority than the first in queue
	int hasLowerPriority = (
		(
			*(
				(**queuePtr).block
			)
		).priority
		>
		(
			*(
				(*listItem).block
			)
		).priority
	);

	if (hasLowerPriority) {
		//Set listItem as first in queue
		(*listItem).next = (*queuePtr);
		(*queuePtr) = listItem;
		return;
	}

	rpthread_listItem_t* item = (*queuePtr);

	//If listItem has a lower priority than the itemPtr
	hasLowerPriority = (
		(
			*(
				(*item).block
			)
		).priority
		>
		(
			*(
				(*listItem).block
			)
		).priority
	);

	//Iterate until next has been in use longer than listItem
	while (hasLowerPriority) {
		item = (*item).next;
		hasLowerPriority = (
			(
				*(
					(*item).block
				)
			).priority
			>
			(
				*(
					(*listItem).block
				)
			).priority
		);
	}

	//Insert listItem
	(*listItem).next = (*item).next;
	(*item).next = listItem;
	return;
}

void restoreBlockedToQueue(rpthread_mutex_t* mutex) {
	//Pop and add each to queue
	while ((*mutex).blocked != NULL) {
		//Pop
		rpthread_listItem_t* listItem = (*mutex).blocked;
		(*mutex).blocked = (*(*mutex).blocked).next;

		//Return to queue
		#ifndef MLFQ //If STCF
			insertIntoSTCF(listItem);
		#else //If MLFQ
			insertIntoMLFQ(listItem, 0); //might need to adjust this
		#endif
	}
}

// Feel free to add any other functions you need
