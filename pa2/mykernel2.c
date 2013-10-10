/* mykernel2.c: your portion of the kernel (last modified 10/18/09)
 *
 *  Below are procedures that are called by other parts of the kernel.
 *  Your ability to modify the kernel is via these procedures.  You may
 *  modify the bodies of these procedures any way you wish (however,
 *  you cannot change the interfaces).
 *
 */

#include "aux.h"
#include "sys.h"
#include "mykernel2.h"

#define TIMERINTERVAL 100000  /* in ticks (tick = 10 msec) */

#define QUEUESIZE 1000
/*  A sample process table.  You may change this any way you wish.
 */

static struct {
  int valid;    /* is this entry valid: 1 = yes, 0 = no */
  int pid;    /* process id (as provided by kernel) */
} proctab[MAXPROCS];

typedef struct{
  int q[QUEUESIZE -1];
  int first;
  int last;
  int count;
}queue;

static queue pid_queue;


void init_queue(queue *q)
{
  q->first = 0;
  q->last = QUEUESIZE - 1;
  q->count = 0;
}

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

int lifo_dequeue(queue *q)
{
  int x;

  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  else{
    x = q->q[ q->last];
    q->last = (q->last - 1) % QUEUESIZE;
    q->count = q->count -1;
  }

  return(x);
}

int empty(queue *q)
{
  if (q->count <= 0) return 1;
  else return 0;
}

void print_queue(queue *q)
{
  int i,j;

  i=q->first;

  while (i != q->last) {
          Printf("%c ",q->q[i]);
          i = (i+1) % QUEUESIZE;
  }

  Printf("%2d ",q->q[i]);
  Printf("\n");
}
/*  InitSched () is called when kernel starts up.  First, set the
 *  scheduling policy (see sys.h).  Make sure you follow the rules
 *  below on where and how to set it.  Next, initialize all your data
 *  structures (such as the process table).  Finally, set the timer
 *  to interrupt after a specified number of ticks.
 */

void InitSched ()
{
  int i;
  /* First, set the scheduling policy.  You should only set it
   * from within this conditional statement.  While you are working
   * on this assignment, GetSchedPolicy will return NOSCHEDPOLICY,
   * and so the condition will be true and you may set the scheduling
   * policy to whatever you choose (i.e., you may replace ARBITRARY).
   * After the assignment is over, during the testing phase, we will
   * have GetSchedPolicy return the policy we wish to test, and so
   * the condition will be false and SetSchedPolicy will not be
   * called, thus leaving the policy to whatever we chose to test.
   */
  if (GetSchedPolicy () == NOSCHEDPOLICY) { /* leave as is */
    SetSchedPolicy (LIFO);   /* set policy here */
  }

  /* Initialize all your data structures here */
  for (i = 0; i < MAXPROCS; i++) {
    proctab[i].valid = 0;
  }
  /* Initialize FIFO Queue; */
  init_queue(&pid_queue);
  /* Set the timer last */
  SetTimer (TIMERINTERVAL);
}


/*  StartingProc (pid) is called by the kernel when the process
 *  identified by pid is starting.  This allows you to record the
 *  arrival of a new process in the process table, and allocate
 *  any resources (if necessary).  Returns 1 if successful, 0 otherwise.
 */

int StartingProc (pid)
  int pid;
{
  int i;

  for (i = 0; i < MAXPROCS; i++) {
    if (! proctab[i].valid) {
      proctab[i].valid = 1;
      proctab[i].pid = pid;
      enqueue(&pid_queue, pid);
      Printf("starting Proc\n");
      print_queue(&pid_queue);

      return (1);
    }
  }

  Printf ("Error in StartingProc: no free table entries\n");
  return (0);
}


/*  EndingProc (pid) is called by the kernel when the process
 *  identified by pid is ending.  This allows you to update the
 *  process table accordingly, and deallocate any resources (if
 *  necessary).  Returns 1 if successful, 0 otherwise.
 */


int EndingProc (pid)
  int pid;
{
  int i;

  for (i = 0; i < MAXPROCS; i++) {
    if (proctab[i].valid && proctab[i].pid == pid) {
      proctab[i].valid = 0;
      return (1);
    }
  }

  Printf ("Error in EndingProc: can't find process %d\n", pid);
  return (0);
}


/*  SchedProc () is called by kernel when it needs a decision for
 *  which process to run next.  It calls the kernel function
 *  GetSchedPolicy () which will return the current scheduling policy
 *  which was previously set via SetSchedPolicy (policy). SchedProc ()
 *  should return a process id, or 0 if there are no processes to run.
 */

int SchedProc ()
{
  int i;
  int fifo_pid;
  int lifo_pid;

  switch (GetSchedPolicy ()) {

  case ARBITRARY:

    for (i = 0; i < MAXPROCS; i++) {
      if (proctab[i].valid) {
        return (proctab[i].pid);
      }
    }
    break;

  case FIFO:
    if ( !empty(&pid_queue) ){
      fifo_pid = dequeue(&pid_queue);
      return fifo_pid;
    }

    break;

  case LIFO:
    if(proctab[0].valid){
      DoSched();
      return proctab[0].pid;
    }
    break;

  case ROUNDROBIN:

    /* your code here */

    break;

  case PROPORTIONAL:

    /* your code here */

    break;

  }

  return (0);
}


/*  HandleTimerIntr () is called by the kernel whenever a timer
 *  interrupt occurs.
 */

void HandleTimerIntr ()
{
  SetTimer (TIMERINTERVAL);

  switch (GetSchedPolicy ()) {  /* is policy preemptive? */

  case ROUNDROBIN:    /* ROUNDROBIN is preemptive */
  case PROPORTIONAL:    /* PROPORTIONAL is preemptive */

    DoSched ();   /* make scheduling decision */
    break;

  default:      /* if non-preemptive, do nothing */
    break;
  }
}

/*  MyRequestCPUrate (pid, m, n) is called by the kernel whenever a process
 *  identified by pid calls RequestCPUrate (m, n).  This is a request for
 *  a fraction m/n of CPU time, effectively running on a CPU that is m/n
 *  of the rate of the actual CPU speed.  m of every n quantums should
 *  be allocated to the calling process.  Both m and n must be greater
 *  than zero, and m must be less than or equal to n.  MyRequestCPUrate
 *  should return 0 if successful, i.e., if such a request can be
 *  satisfied, otherwise it should return -1, i.e., error (including if
 *  m < 1, or n < 1, or m > n).  If MyRequestCPUrate fails, it should
 *  have no effect on scheduling of this or any other process, i.e., as
 *  if it were never called.
 */

int MyRequestCPUrate (pid, m, n)
  int pid;
  int m;
  int n;
{
  /* your code here */

  return (0);
}
