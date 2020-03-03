#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include <sys/time.h>
#include <unistd.h>

static ucontext_t c[3];
static ucontext_t* current;
struct itimerval timer;

int state = 0;

static void f1(void) {
	printf("f1\n");
	while(1);
};
static void f2(void) {
	printf("f2\n");
	while(1);
};

void timerHandler(int sigNum) {
	printf("Swapping from %d to ", state);
	printf("%d\n", ++state);
	
	ucontext_t* old = current;
	if (state < 3) {
		current = c + state;
	} else {
		current = c;
	}
	swapcontext(c, current);
	printf("Timer handler end\n");
	//prints when swapping back to c[0], so inside main context
}

int main(int argc, char** argv)
{
	current = c;
	char f1Stack[4096], f2Stack[4096]; //Allocate stack space for both contexts

	getcontext(c+1); //Sets current context into c1 to initialize all values
	c[1].uc_stack.ss_sp = f1Stack; //Sets stack pointer
	c[1].uc_stack.ss_size = sizeof(f1Stack); //Declares amount of available space
	makecontext(c+1, f1, 0); //Creates context in c1 doing f1

	getcontext(c+2);
	c[2].uc_stack.ss_sp = f2Stack;
	c[2].uc_stack.ss_size = sizeof(f2Stack);
	makecontext(c+2, f2, 0); //Creates context in c2 doing f2

	//Setup timer
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &timerHandler;
	sigaction(SIGVTALRM, &sa, NULL);

	//Triggers after 250ms
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 250000;

	//Triggers on loop every 250ms after first trigger
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 250000;

	//Start schedule
	setitimer(ITIMER_VIRTUAL, &timer, NULL);

	printf("f0\n");
	while (state == 0);
	printf("f0\n");

	return 0;
}
