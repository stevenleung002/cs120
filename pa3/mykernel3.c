/* mykernel.c: your portion of the kernel
 *
 *	Below are procedures that are called by other parts of the kernel.
 *	Your ability to modify the kernel is via these procedures.  You may
 *	modify the bodies of these procedures any way you wish (however,
 *	you cannot change the interfaces).
 */

#include "aux.h"
#include "sys.h"
#include "mykernel3.h"

#define FALSE 0
#define TRUE 1
#define QUEUESIZE 1000

typedef struct{
  int q[QUEUESIZE -1];
  int first;
  int last;
  int pointer;
  int count;
}queue;

// initialize queue
void init_queue(queue *q)
{
  q->first = 0;
  q->last = QUEUESIZE - 1;
  q->count = 0;
  q->pointer = 0;
}

//insert element to queue
void enqueue(queue *q, int x)
{
  if (q->count >= QUEUESIZE)
  Printf("Warning: queue overflow enqueue x=%d\n",x);
  else {
    q->last = (q->last+1) % QUEUESIZE;
    q->q[ q->last ] = x;
    q->count = q->count + 1;
  }
}

//remove element from queue, always the first element
int dequeue(queue *q)
{
  int x;

  if (q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  else {
    x = q->q[ q->first ];
    q->first = (q->first+1) % QUEUESIZE;
    q->count = q->count - 1;
  }

  return(x);
}


// helper method to get last element in queue
int get_queue_last(queue *q)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  return q->q[ q->last ];
}
// helper method to get the first element in queue
int get_queue_first(queue *q)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  return q->q[ q->first ];
}
// helpter method to get next element in queue
int get_queue_next(queue *q)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  int current = q->pointer;
  q->pointer = (q->pointer + 1) % q->count;
  return q->q[current];
}
// helpter method to check whether queue is empty or not
int empty(queue *q)
{
  if (q->count <= 0) return 1;
  else return 0;
}



/*	A sample semaphore table.  You may change this any way you wish.
 */

static struct {
	int valid;	/* Is this a valid entry (was sem allocated)? */
	int value;	/* value of semaphore */
	queue pid_queue;
} semtab[MAXSEMS];


/*	InitSem () is called when kernel starts up.  Initialize data
 *	structures (such as the semaphore table) and call any initialization
 *	procedures here.
 */

void InitSem ()
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {		/* mark all sems free */
		semtab[s].valid = FALSE;
	}
}

/*	MySeminit (p, v) is called by the kernel whenever the system
 *	call Seminit (v) is called.  The kernel passes the initial
 * 	value v, along with the process ID p of the process that called
 *	Seminit.  MySeminit should allocate a semaphore (find a free entry
 *	in semtab and allocate), initialize that semaphore's value to v,
 *	and then return the ID (i.e., index of the allocated entry).
 */

int MySeminit (p, v)
	int p, v;
{
	int s;

	/* modify or add code any way you wish */

	for (s = 0; s < MAXSEMS; s++) {
		if (semtab[s].valid == FALSE) {
			break;
		}
	}
	if (s == MAXSEMS) {
		Printf ("No free semaphores\n");
		return (-1);
	}

	semtab[s].valid = TRUE;
	semtab[s].value = v;
	init_queue(&(semtab[s].pid_queue));
	return (s);
}

/*	MyWait (p, s) is called by the kernel whenever the system call
 *	Wait (s) is called.
 */

void MyWait (p, s)
	int p, s;
{
	/* modify or add code any way you wish */
	semtab[s].value--;
	Printf("process %d set semtab %d value is %d \n", p, s, semtab[s].value);
	if (semtab[s].value < 0)
	{
		enqueue(&(semtab[s].pid_queue), p);
		Printf("\nprocess %d get blocked at position %d \n\n", p, s);
		Block(p);
	}
}

/*	MySignal (p, s) is called by the kernel whenever the system call
 *	Signal (s) is called.
 */

void MySignal (p, s)
	int p, s;
{
	/* modify or add code any way you wish */
	semtab[s].value++;
	Printf("\nprocess %d signal semtab %d value to %d \n\n", p, s, semtab[s].value);
	if (semtab[s].value <= 0)
	{
		int unblock_pid = dequeue(&(semtab[s].pid_queue));
		Unblock(unblock_pid);
	}
}

