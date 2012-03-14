/*
  gcc -Wall -Wextra thread_test.c -g -lpthread 
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <boost/preprocessor/array/elem.hpp>
#include <boost/preprocessor/repetition.hpp>

#define KB(x) (x * 1024)

#if 0
# define BOOST_PP_TUPLE_REM(size) BOOST_PP_TUPLE_REM_OO((size))
# define BOOST_PP_TUPLE_REM_OO(par) BOOST_PP_TUPLE_REM_I ## par
# define BOOST_PP_TUPLE_REM_I(size) BOOST_PP_TUPLE_REM_ ## size

# define BOOST_PP_TUPLE_REM_0()
# define BOOST_PP_TUPLE_REM_1(a) a
# define BOOST_PP_TUPLE_REM_2(a, b) a, b
# define BOOST_PP_TUPLE_REM_3(a, b, c) a, b, c
# define BOOST_PP_TUPLE_REM_4(a, b, c, d) a, b, c, d
#endif

/* we want to call this func from a thread. */
int worker_func(int x, void* y, int z, void* a) {
	return z;
}



#if 0

typedef struct {
	pthread_attr_t attr;
	pthread_t thread;
	int result;
	int x; 
	void* y; 
	int z;
} worker_func_thread_data;

static void* worker_func_child_thread(void* data) {
	worker_func_thread_data *child_thread_data = (worker_func_thread_data*)data;
	child_thread_data->result = worker_func(child_thread_data->x, child_thread_data->y, child_thread_data->z);
	return NULL;
}
/* returns NULL on success, otherwise error message string 
 * if an error happens, the pthread_attr_t member of ti gets
 * automatically cleaned up. */
static const char* worker_func_thread_launcher(size_t stacksize, void ** vti, int x, void* y, int z) {
	const char* errmsg = NULL;
	worker_func_thread_data** ti = (worker_func_thread_data**)vti;
	*ti = calloc(1, sizeof(worker_func_thread_data));
	if(!(*ti)) goto ret;
	(*ti)->x = x;
	(*ti)->y = y;
	(*ti)->z = z;
	
	if((errno = pthread_attr_init(&(*ti)->attr))) {
		errmsg = "pthread_attr_init";
		goto err;
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
	err:
	free(*ti);
	(*ti) = NULL;
	goto ret;
}

static const char* worker_func_wait(int* result, void** vti) {
	worker_func_thread_data** ti = (worker_func_thread_data**)vti;
	const char* errmsg = NULL;

	if((errno = pthread_join((*ti)->thread, NULL))) {
		errmsg = "pthread_join";
		pthread_attr_destroy(&(*ti)->attr);
		goto ret;
	}
	if((errno = pthread_attr_destroy(&(*ti)->attr))) {
		errmsg = "pthread_attr_destroy";
	}
	*result = (*ti)->result;
	ret:
	free(*ti);
	*ti = NULL;
	return errmsg;
}
#endif

#define THREAD_WRAPPER_EXPAND_FUNCTION_ARGS(z, n, data) child_thread_data->BOOST_PP_ARRAY_ELEM(n,data)
#define THREAD_WRAPPER_ASSIGN(z, n, data) (*ti)->BOOST_PP_ARRAY_ELEM(n,data) = BOOST_PP_ARRAY_ELEM(n,data);
#define THREAD_WRAPPER_EXPAND_SEMICOLON(z, n, data) BOOST_PP_ARRAY_ELEM(n,data);
#define THREAD_WRAPPER(returntype, function, types_and_args, argcount, args) \
typedef struct { \
	pthread_attr_t attr; \
	pthread_t thread; \
	returntype result; \
	BOOST_PP_REPEAT(argcount, THREAD_WRAPPER_EXPAND_SEMICOLON, (argcount, types_and_args)) \
} function ## _thread_data; \
static void* function ## _child_thread(void* data) { \
	function ## _thread_data *child_thread_data = (function ## _thread_data *)data; \
	child_thread_data->result = function(BOOST_PP_ENUM(argcount, THREAD_WRAPPER_EXPAND_FUNCTION_ARGS, (argcount, args))); \
	return NULL; \
}\
static const char* function ## _thread_launcher(size_t stacksize, void ** vti, BOOST_PP_TUPLE_REM_I(argcount) types_and_args) { \
	const char* errmsg = NULL; \
	function ## _thread_data** ti = (function ## _thread_data**)vti; \
	*ti = calloc(1, sizeof(function ## _thread_data)); \
	if(!(*ti)) goto ret; \
	BOOST_PP_REPEAT(argcount, THREAD_WRAPPER_ASSIGN, (argcount, args)) \
	if((errno = pthread_attr_init(&(*ti)->attr))) { \
		errmsg = "pthread_attr_init"; \
		goto err; \
	} \
	\
	if((errno = pthread_attr_setstacksize(&(*ti)->attr, stacksize))) { \
		errmsg = "pthread_attr_setstacksize"; \
		goto pt_err_attr; \
	}\
	if((errno = pthread_create(&(*ti)->thread, &(*ti)->attr, function ## _child_thread, *ti))) { \
		errmsg = "pthread_create"; \
		goto pt_err_attr; \
	} \
	 \
	ret: \
	return errmsg; \
	 \
	pt_err_attr: \
	pthread_attr_destroy(&(*ti)->attr); \
	err: \
	free(*ti); \
	(*ti) = NULL; \
	goto ret; \
} \
\
static const char* function ## _wait(returntype * result, void** vti) { \
	function ## _thread_data** ti = (function ## _thread_data**)vti; \
	const char* errmsg = NULL; \
\
	if((errno = pthread_join((*ti)->thread, NULL))) { \
		errmsg = "pthread_join"; \
		pthread_attr_destroy(&(*ti)->attr); \
		goto ret; \
	} \
	if((errno = pthread_attr_destroy(&(*ti)->attr))) { \
		errmsg = "pthread_attr_destroy"; \
	} \
	*result = (*ti)->result; \
	ret: \
	free(*ti); \
	*ti = NULL; \
	return errmsg; \
}

THREAD_WRAPPER(int, worker_func, (int x, void* y, int z, void* a), 4, (x, y, z, a));


#define THREAD_LAUNCH(stacksize, name, function, argcount, args) \
function ## _thread_launcher(stacksize, &(name), BOOST_PP_TUPLE_REM_I(argcount) args )


#define THREAD_WAIT(presult, name, function) \
	function ## _wait(presult, &(name))

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
