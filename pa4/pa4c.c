/* Programming Assignment 4: Exercise C
 *
 * You are ready to begin building a user-level thread package.  Your task is
 * to implement the functions: MyInitThreads (), MySpawnThread (func, param),
 * MyYieldThread (t), MyGetThread (), MySchedThread (), and MyExitThread ().
 * They should work in a general way, supporting MAXTHREADS active threads.
 * Note that a program may spawn more than MAXTHREADS threads, as long as
 * no more than MAXTHREADS threads are active at any point in time.
 *
 * You are given a test program below.  It currently references thread
 * functions that are properly implemented, i.e., SpawnThread, YieldThread,
 * etc., which mirror the ones you are to implement.  Eventually, you
 * should replace all of them with reference to your functions by simply
 * prefixing each one with "My", e.g., "SpawnThread" becomes "MySpawnThread".
 * Note that you cannot mix your functions with the working ones, so you
 * must replace all of them.
 *
 *
 * Some notes:
 *
 * 1. MyInitThreads () should initialize all your thread management data
 * structures.  Here is where you may wish to reserve stack space FOR THE
 * MAXIMUM NUMBER OF THREADS that may be active at any one time (MAXTHREADS).
 *
 * 2. MySpawnThread should return the id of the thread just spawned (assuming
 * no errors).  In Exercise B, it was assumed the return value was 1 because it
 * could only spawn a single thread (with id 1); this needs to be generalized
 * to any value between 0 and MAXTHREADS-1.  If there is an error, such as if
 * there are already MAXTHREADS active threads (and no more can be created
 * until one or more exit), MySpawnThread should simply return -1.
 * IMPORTANT: Threads id's should be integers that are assigned in increasing
 * order.  The initial thread (that exists by default) is thread 0.  The first
 * time MySpawnThread is called, it should create thread 1, and each subsequent
 * (successful) call to MySpawnThread, regardless of which thread makes the
 * call, should assign id's 2, 3, ... 9, i.e., up to and including MAXTHREADS-1
 * (for MAXTHREADS equal to 10 in this example).  Values should be reused
 * AFTER having reached MAXTHREADS-1, again starting from 0 and incrementing
 * by 1, but a value that is in use should be skipped over (and thus not
 * assigned to a new thread since it is already the id of an active one).
 * The result is that each active thread will have a unique id between 0 and
 * MAXTHREADS-1 inclusive.  Note the increasing order of assignment and
 * incrementing by 1 (this is very important, as our test programs will expect
 * this order of id assignment).
 *
 * 3. MyYieldThread (t) needs to be generalized so that any thread can yield
 * to any other thread, including itself.  Also, the id of the calling thread
 * must be properly returned, or -1 if t is invalid.  To clarify this further,
 * consider a program of many threads, two of which are threads 3 and 7.
 *
 * Thread 3 contains the statement:
 *
 * x = MyYieldThread (7);	// causes thread 3 to yield to thread 7
 *
 * Thread 7 contains the statement:
 *
 * x = MyYieldThread (t);	// causes thread 7 to yield to thread t
 *
 * Assume that at some point in the past, thread 7 had run and had executed
 * its yield statement as shown above.  To what it yields to is not shown
 * as it depends on the value of t.  If t equalled 3, then control would
 * have gone to thread 3.  If it were another value, another thread would
 * have gotten control.  Regardless, assume that thread 3 is now running,
 * and executes its yield statement as shown above.  At this point, control
 * is given to thread 7, which returns from its yield statement with x set
 * to 3 (because it was thread 3 that yielded to thread 7).  The value of
 * x that will be set in thread 3 when its yield statement returns will
 * depend on whatever other thread eventually yields to it.
 *
 * 4. MySchedThread () is similar to MyYieldThread, except that MySchedThread
 * determines which thread to yield to, rather than this being specified via
 * a parameter as in MyYieldThread.  MySchedThread does not return any value.
 * If MySchedThread is called and there is no thread to run, it should call
 * Exit () so that the Umix process properly completes.
 *
 * 5. IMPORTANT: MySchedThread should implement the FIFO (first-in-first-out)
 * scheduling discipline.  Thus, if a thread calls MySchedThread, it should
 * be placed at the end of a queue, and whichever thread is at the front
 * should be selected for execution.  If a thread calls MyYieldThread (t),
 * then the calling thread should be placed at the end of the queue, and t
 * should be removed from the queue, regardless of its position, and treated
 * as if it were at the front, i.e., selected for execution and when it gives
 * up the CPU, it should go to the end of the queue.
 *
 * 6. MyExitThread () should cause the currently running thread to exit,
 * i.e., it should never run again, and its resources, such as its entry in
 * the thread table, should be reclaimed so that another thread may use them.
 * Finally, it should call MySchedThread () to pass control to another active
 * thread.
 *
 * WHAT TO TURN IN
 *
 * You must turn in one file: mythreads.c, which contains your user-level
 * thread implementation.
 *
 * Your programs will be tested with various Main programs that will exercise
 * your threads implementation.  As always, make sure you do as much rigorous
 * testing yourself to be sure your implementations are robust.
 *
 * While this assignment may be conceptually difficult, it does not require
 * a large amount of code.  The solution is roughly 100 lines of C code
 * (which include some comments) more than the version of mythreads.c that
 * was given to you.
 *
 * Good luck!
 */

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

#define NUMYIELDS	5

static int square, cube;	/* global variables, shared by threads */

void Main ()
{
	int i, t, me;
	void printSquares (), printCubes ();

	InitThreads ();

	me = MyGetThread ();
	t = MySpawnThread (printSquares, me); // t == 1
	t = MySpawnThread (printCubes, t); // t == 2

	for (i = 0; i < NUMYIELDS; i++) {
		MyYieldThread (t);
		Printf ("T%d: square = %d, cube = %d\n", me, square, cube);
	}

	ExitThread ();
}

void printSquares (t)
	int t;				/* thread to yield to */
{
	int i;

	for (i = 0; i < NUMYIELDS; i++) {
		square = i * i;
		Printf ("T%d: %d squared = %d\n", MyGetThread (), i, square);
		MyYieldThread (t);
	}
}

void printCubes (t)
	int t;				/* thread to yield to */
{
	int i;

	for (i = 0; i < NUMYIELDS; i++) {
		cube = i * i * i;
		Printf ("T%d: %d cubed = %d\n", MyGetThread (), i, cube);
		MyYieldThread (t);
	}
}
