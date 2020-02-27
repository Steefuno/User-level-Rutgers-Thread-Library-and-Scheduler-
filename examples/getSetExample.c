#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

//Infinite loop of reverting to a context
int main(int argc, char** argv)
{
	ucontext_t c;

	printf("Hello world\n");

	getcontext(&c); //sets current context into c
	sleep(1);
	printf("Hey\n"); //output
	setcontext(&c); //set current context to stored contex from c

	printf("Wha\n");

	return 0;
}
