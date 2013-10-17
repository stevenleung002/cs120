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

#define TIMERINTERVAL 1  /* in ticks (tick = 10 msec) */

#define QUEUESIZE 1000
/*  A sample process table.  You may change this any way you wish.
 */

static struct {
  int valid;    /* is this entry valid: 1 = yes, 0 = no */
  int pid;    /* process id (as provided by kernel) */
  int stoped; /* check whether process stoped, 1 = yes, 0 = no*/
  double requested; /* requested cpu ratio*/
  double utilization; /* utiliaztion ratio */
  long alive_slot; /* process alive slot count */
  long ran_slot; /* process ran slot*/
} proctab[MAXPROCS];

/* every time, MyRequestCPUrate is called, we set that process's request ratio */
void set_requested_ratio(int pid, int m, int n){
  for (int i = 0; i < MAXPROCS; i++) {
    if (proctab[i].valid == 0){
      return;
    }
    else if(proctab[i].pid == pid) {
      double request = (double)m / n;
      proctab[i].requested = request;

      return;
    }
    else{
      continue;
    }
  }
}

/* every time SchedProc invoked, we refresh every started process's utilization*/
void refresh_slot()
{
  for (int i = 0; i < MAXPROCS; i++) {
    if (proctab[i].valid == 0){
      return;
    }
    else{
      proctab[i].alive_slot += 1;
    }
    proctab[i].utilization = (double)proctab[i].ran_slot / proctab[i].alive_slot;
  }
}

/* calculate which process is the unfair treated one*/
int get_unfair_pid()
{
  int unfair_pid = 0;
  int unfair_pid_index = 0;

  double smallest_compute_ratio = 1000;

  for(int i = 0; i < MAXPROCS; i++){
    if (proctab[i].stoped == 1) continue;

    double ratio = proctab[i].utilization / proctab[i].requested;

    if(ratio != ratio)
      ratio = 0;

    if (proctab[i].valid == 0){
      proctab[unfair_pid_index].ran_slot += 1;
      double utilization = (double)proctab[unfair_pid_index].ran_slot / proctab[unfair_pid_index].alive_slot;
      proctab[unfair_pid_index].utilization = utilization;

      return unfair_pid;
    }
    else if( ratio < smallest_compute_ratio){
      smallest_compute_ratio = ratio;
      unfair_pid = proctab[i].pid;
      unfair_pid_index = i;
    }
  }

  return unfair_pid;
}

/* queue is for LIFO FIFO RoundRobin */
typedef struct{
  int q[QUEUESIZE -1];
  int first;
  int last;
  int pointer;
  int count;
}queue;

static queue pid_queue;

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

//remove element from queue, always the last element, like stack
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
// helper method to delete a specific pid from queue
void delete_pid(queue *q, int pid)
{
  if(q->count <= 0) Printf("Warning: empty queue dequeue.\n");
  else{
    for(int i = 0; i < q->count; i++)
    {
      if(q->q[i] == pid)
      {
        while(i <= q->last)
        {
          q->q[i] = q->q[i+1];
          i++;
        }
        q->last = (q->last - 1) % QUEUESIZE;
        q->pointer = q->last;
        q->count = q->count -1;
      }
    }
  }
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
    SetSchedPolicy (ROUNDROBIN);   /* set policy here */
  }

  /* Initialize all your data structures here */
  for (i = 0; i < MAXPROCS; i++) {
    proctab[i].valid = 0;
    proctab[i].stoped = 0;
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

  switch (GetSchedPolicy ()) {
    case ARBITRARY:

      for (i = 0; i < MAXPROCS; i++) {
        if (! proctab[i].valid) {
          proctab[i].valid = 1;
          proctab[i].pid = pid;
          return (1);
        }
      }

      break;

    case FIFO:
      enqueue(&pid_queue, pid);
      return (1);

      break;

    case LIFO:
      enqueue(&pid_queue, pid);
      DoSched();
      return (1);

      break;

    case ROUNDROBIN:
      enqueue(&pid_queue, pid);
      return (1);

      break;

    case PROPORTIONAL:
      for (i = 0; i < MAXPROCS; i++) {
        if (! proctab[i].valid) {
          proctab[i].valid = 1;
          proctab[i].pid = pid;
          proctab[i].ran_slot = 0;
          proctab[i].alive_slot = 0;
          return (1);
        }
      }
      break;

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

  switch (GetSchedPolicy ()) {
    case ARBITRARY:
      for (i = 0; i < MAXPROCS; i++) {
        if (proctab[i].valid && proctab[i].pid == pid) {
          proctab[i].valid = 0;
          return (1);
        }
      }

      break;

    case FIFO:
      dequeue(&pid_queue);
      return (1);
      break;

    case LIFO:
      lifo_dequeue(&pid_queue);
      return(1);

      break;

    case ROUNDROBIN:
      delete_pid(&pid_queue, pid);
      return(1);
      break;

    case PROPORTIONAL:
      for (i = 0; i < MAXPROCS; i++) {
        if (proctab[i].valid && proctab[i].pid == pid) {
          proctab[i].valid = 0;
          proctab[i].stoped = 1;
          return (1);
        }
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
  int ror_pid;
  int current_pid;
  int prop_pid;

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
      fifo_pid = get_queue_first(&pid_queue);
      return fifo_pid;
    }

    break;

  case LIFO:
    if( !empty(&pid_queue) ){
      lifo_pid = get_queue_last(&pid_queue);
      return lifo_pid;
    }
    break;

  case ROUNDROBIN:
    if ( !empty(&pid_queue) ){
      ror_pid = get_queue_next(&pid_queue);
      return ror_pid;
    }
    break;

  case PROPORTIONAL:
   // Printf("\n Scheduling Proc \n");
    refresh_slot();
   // Printf("refreshed Proc slot \n");
    prop_pid = get_unfair_pid();
   // Printf("scheduling Proc %d \n", prop_pid);
    return prop_pid;

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
  if (m < 1 || n < 1 || m > n){
    return -1;
  }
  else{
    set_requested_ratio(pid, m, n);
  }
  return (0);
}
