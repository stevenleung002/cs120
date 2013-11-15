/*	User-level thread system
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

static int MyInitThreadsCalled = 0;	/* 1 if MyInitThreads called, else 0 */

static struct thread {			/* thread table */
	int valid;			/* 1 if entry is valid, else 0 */
	jmp_buf env;			/* current context */
} thread[MAXTHREADS];

#define STACKSIZE	65536		/* maximum size of thread stack */

/*	MyInitThreads () initializes the thread package.  Must be the first
 *	function called by any user program that uses the thread package.
 */

void MyInitThreads ()
{
	int i;

	thread[0].valid = 1;			/* the initial thread is 0 */

	for (i = 1; i < MAXTHREADS; i++) {	/* all other threads invalid */
		thread[i].valid = 0;
	}

	MyInitThreadsCalled = 1;
}

/*	MySpawnThread (func, param) spawns a new thread to execute
 *	func (param), where func is a function with no return value and
 *	param is an integer parameter.  The new thread does not begin
 *	executing until another thread yields to it.
 */

int MySpawnThread (func, param)
	void (*func)();		/* function to be executed */
	int param;		/* integer parameter */
{
	if (! MyInitThreadsCalled) {
		Printf ("MySpawnThread: Must call MyInitThreads first\n");
		Exit ();
	}

	if (setjmp (thread[0].env) == 0) {	/* save context of thread 0 */

		/* The new thread will need stack space.  Here we use the
		 * following trick: the new thread simply uses the current
		 * stack, and so there is no need to allocate space.  However,
		 * to ensure that thread 0's stack may grow and (hopefully)
		 * not bump into thread 1's stack, the top of the stack is
		 * effectively extended automatically by declaring a local
		 * variable (a large "dummy" array).  This array is never
		 * actually used; to prevent an optimizing compiler from
		 * removing it, it should be referenced.
		 */

		char s[STACKSIZE];	/* reserve space for thread 0's stack */
		void (*f)() = func;	/* f saves func on top of stack */
		int p = param;		/* p saves param on top of stack */

		if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
			Printf ("Stack space reservation failed\n");
			Exit ();
		}

		if (setjmp (thread[1].env) == 0) {	/* save context of 1 */
			longjmp (thread[0].env, 1);	/* back to thread 0 */
		}

		/* here when thread 1 is scheduled for the first time */

		(*f) (p);			/* execute func (param) */

		MyExitThread ();		/* thread 1 is done - exit */
	}

	thread[1].valid = 1;	/* mark the entry for the new thread valid */

	return (1);		/* done spawning, return new thread id */
}

/*	MyYieldThread (t) causes the running thread to yield to thread t.
 *	Returns id of thread that yielded to t (i.e., the thread that called
 *	MyYieldThread), or -1 if t is an invalid id.
 */

int MyYieldThread (t)
	int t;				/* thread being yielded to */
{
	if (! MyInitThreadsCalled) {
		Printf ("MyYieldThread: Must call MyInitThreads first\n");
		Exit ();
	}

	if (! thread[t].valid) {
		Printf ("MyYieldThread: Thread %d does not exist\n", t);
		return (-1);
	}

        if (setjmp (thread[1-t].env) == 0) {
                longjmp (thread[t].env, 1);
        }
}

/*	MyGetThread () returns id of currently running thread.
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyGetThread: Must call MyInitThreads first\n");
		Exit ();
	}

}

/*	MySchedThread () causes the running thread to simply give up the
 *	CPU and allow another thread to be scheduled.  Selecting which
 *	thread to run is determined here.  Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads).
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MySchedThread: Must call MyInitThreads first\n");
		Exit ();
	}
}

/*	MyExitThread () causes the currently running thread to exit.
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("MyExitThread: Must call MyInitThreads first\n");
		Exit ();
	}
}
