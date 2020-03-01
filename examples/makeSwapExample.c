nclude <stdio.h>
#include <ucontext.h>

static ucontext_t c0, c1, c2;

static void f1(void)
{
	printf("f1 start, swapping to c2\n");
	swapcontext(&c1, &c2); //Swap to f2
	printf("f1 end, uc_link to c0\n");
}

static void f2(void)
{
	printf("f2 start, swapping to c1\n");
	swapcontext(&c2, &c1); //Swap to f1
	printf("f2 end, uc_link to c1\n");
}

int main(int argc, char** argv)
{
	char f1Stack[4096], f2Stack[4096]; //Allocate stack space for both contexts

	getcontext(&c1); //Sets current context into c1 to initialize all values
	c1.uc_stack.ss_sp = f1Stack; //Sets stack pointer
	c1.uc_stack.ss_size = sizeof(f1Stack); //Declares amount of available space
	c1.uc_link = &c0; //After c1 returns, goes to c0
	makecontext(&c1, f1, 0); //Creates context in c1 doing f1

	getcontext(&c2);
	c2.uc_stack.ss_sp = f2Stack;
	c2.uc_stack.ss_size = sizeof(f2Stack);
	c2.uc_link = &c1; //After c2 returns, goes to c1
	makecontext(&c2, f2, 0); //Creates context in c2 doing f2

	printf("swapping c0 to c2\n");
	swapcontext(&c0, &c2); //Swaps c0 with c2 to do f2

	printf("main end, exiting\n");
	return 0;
}
