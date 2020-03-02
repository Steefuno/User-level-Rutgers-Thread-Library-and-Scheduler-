#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>

static ucontext_t c0, cS, c1, c2;
static ucontext_t* current = NULL;
struct itimerval timer;

static int state = 1;

static void f1(void) {
	while(1);
};
static void f2(void) {
	while(1);
};

static void scheduler(void) {
	while(1) {
		//Pick next function
		state = ++state % 2;

		ucontext_t* old = current;
		printf("Swap: %x", current);

		if (state == 0) {
			current = &c1;
		} else {
			current = &c2;
		}

		printf("\t%x\n", current);

		//Setup timer to interrupt
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		//Start
		swapcontext(&cS, current);
	}
}
void timerHandler(int sigNum) {
	printf("TIME\n");
	swapcontext(current, &cS);
}

int main(int argc, char** argv)
{
	current = &c0;
	char fSStack[4096], f1Stack[4096], f2Stack[4096]; //Allocate stack space for both contexts

	getcontext(&c1); //Sets current context into c1 to initialize all values
	c1.uc_stack.ss_sp = f1Stack; //Sets stack pointer
	c1.uc_stack.ss_size = sizeof(f1Stack); //Declares amount of available space
	c1.uc_link = &cS; //After c1 returns, goes to scheduler
	makecontext(&c1, f1, 0); //Creates context in c1 doing f1

	getcontext(&c2);
	c2.uc_stack.ss_sp = f2Stack;
	c2.uc_stack.ss_size = sizeof(f2Stack);
	c2.uc_link = &cS;
	makecontext(&c2, f2, 0); //Creates context in c2 doing f2

	getcontext(&cS);
	cS.uc_stack.ss_sp = fSStack;
	cS.uc_stack.ss_size = sizeof(fSStack);
	cS.uc_link = NULL;
	makecontext(&cS, scheduler, 0);

	//Setup timer
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &timerHandler;
	sigaction(SIGVTALRM, &sa, NULL);

	//Triggers after 250ms
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 250000;

	//Triggers on loop every 250ms after first trigger
	/*
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 250000;
	*/

	printf("%x %x %x\n", &c1, &c2, &cS);

	//Start scheduler
	swapcontext(&c0, &cS);

	return 0;
}
