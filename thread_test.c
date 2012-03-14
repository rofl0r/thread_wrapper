/*
  gcc -Wall -Wextra thread_test.c -g -lpthread 
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define KB(x) (x * 1024)


/* we want to call this func from a thread. */
int worker_func(int x, void* y, int z) {
	return z;
}

typedef struct {
	pthread_attr_t attr;
	pthread_t thread;
	int* result;
	int x; 
	void* y; 
	int z;
} worker_func_thread_data;

static void* worker_func_child_thread(void* data) {
	worker_func_thread_data *child_thread_data = (worker_func_thread_data*)data;
	*child_thread_data->result = worker_func(child_thread_data->x, child_thread_data->y, child_thread_data->z);
	return NULL;
}

/* returns NULL on success, otherwise error message string 
 * if an error happens, the pthread_attr_t member of ti gets
 * automatically cleaned up. */
static const char* worker_func_thread_launcher(worker_func_thread_data** ti, int* result, size_t stacksize, int x, void* y, int z) {
	const char* errmsg = NULL;
	*ti = calloc(1, sizeof(worker_func_thread_data));
	(*ti)->result = result;
	(*ti)->x = x;
	(*ti)->y = y;
	(*ti)->z = z;
	
	if((errno = pthread_attr_init(&(*ti)->attr))) {
		errmsg = "pthread_attr_init";
		goto ret;
	}

	if((errno = pthread_attr_setstacksize(&(*ti)->attr, stacksize))) {
		errmsg = "pthread_attr_setstacksize";
		goto pt_err_attr;
	}
	if((errno = pthread_create(&(*ti)->thread, &(*ti)->attr, worker_func_child_thread, *ti))) {
		errmsg = "pthread_create";
		goto pt_err_attr;
	}
	
	ret:
	return errmsg;
	
	pt_err_attr:
	pthread_attr_destroy(&(*ti)->attr);
	goto ret;
}

static const char* worker_func_wait(worker_func_thread_data** ti, int* result) {
	const char* errmsg = NULL;

	if((errno = pthread_join((*ti)->thread, NULL))) {
		errmsg = "pthread_join";
		pthread_attr_destroy(&(*ti)->attr);
		goto ret;
	}
	if((errno = pthread_attr_destroy(&(*ti)->attr))) {
		errmsg = "pthread_attr_destroy";
	}
	*result = *(*ti)->result;
	ret:
	free(*ti);
	*ti = NULL;
	return errmsg;
}

int main() {
	worker_func_thread_data* child;
	int result;
	const char* errmsg;
	if((errmsg = worker_func_thread_launcher(&child, &result, KB(128), 0, NULL, 1))) goto pt_err;
	if((errmsg = worker_func_wait(&child, &result))) goto pt_err;
	
	printf("workerfunc returned %d\n", result);
	
	return 0;
	pt_err:
	perror(errmsg);
	return 1;
}
