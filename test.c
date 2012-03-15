/*
  gcc -Wall -Wextra test.c -g -lpthread 
 */

#include "thread_wrapper.h"
#include <stdio.h>


#define KB(x) (x * 1024)

/* we want to call this func from a thread. */
int worker_func(int x, void* y, int z, void* a) {
	return z;
}

THREAD_WRAPPER(int, worker_func, 4, (int x, void* y, int z, void* a), (x, y, z, a));

int main() {
	void* child;
	int result;
	const char* errmsg;
	if((errmsg = THREAD_LAUNCH(KB(128), child, worker_func, 4, (0, NULL, 1, (void*) 0xdeadbeef)))) goto pt_err;
	if((errmsg = THREAD_WAIT(&result, child, worker_func))) goto pt_err;
	
	printf("workerfunc returned %d\n", result);
	
	return 0;
	pt_err:
	perror(errmsg);
	return 1;
}
