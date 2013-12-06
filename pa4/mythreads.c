/*  User-level thread system
 *
 */

#include <setjmp.h>

#include "aux.h"
#include "umix.h"
#include "mythreads.h"

#define QUEUESIZE 1000
#define DEBUG 0 
static int MyInitThreadsCalled = 0; /* 1 if MyInitThreads called, else 0 */
static int head = 1;
static int search_from = 1;
static int current_tid = 0;
static int spawning_tid = 0;
typedef struct{
  int q[QUEUESIZE -1];
  int first;
  int last;
  int pointer;
  int count;
}queue;

static queue tid_queue;

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

void print_queue(queue *q){
 if(DEBUG == 1) Printf("\n" );
 int counter = 0;
 while( counter < q->count){
  if(DEBUG == 1) Printf("%d ", q->q[q->first + counter]);
  counter++;
 }
 if(DEBUG == 1) Printf("\n");
}

void move_to_queue_head(queue *q, int t){
  int count = 0; 
  while(count < q->count){
    if(q->q[q->first + count] == t){
      //Printf("hit on %d", (q->first + count));
      if((q->first + count) == q->first){
        break;
      }
      int inner_count = count;
      while((q->first + inner_count) != q->last){
//        if(DEBUG == 1) Printf("replace %d with %d\n");
        q->q[q->first + inner_count] = q->q[q->first + inner_count + 1];
//        print_queue(&tid_queue);
        inner_count += 1;
      }
      if(q->last > 1 && q->first > 0){
        q->last -= 1;
        q->q[q->first - 1] = t;
        q->first -= 1;
      }
    }
    count++;
  }

  
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

static struct thread {      /* thread table */
  int valid;      /* 1 if entry is valid, else 0 */
  jmp_buf env;      /* current context */
  jmp_buf clean_env;
  int clean;
  void *func;
  int param;
} thread[MAXTHREADS];
#define STACKSIZE 65536   /* maximum size of thread stack */

/*  MyInitThreads () initializes the thread package.  Must be the first
 *  function called by any user program that uses the thread package.
 */
void setStackSpace(int pos)
{
  if(pos < 1){
    thread[0].clean = 0;
    longjmp(thread[0].clean_env, 2);
  }
  char s[STACKSIZE];
  if( setjmp(thread[MAXTHREADS - pos].clean_env) == 0){
    if(DEBUG == 1) Printf("thread %d env %d\n", MAXTHREADS - pos, thread[MAXTHREADS - pos].clean_env);
    if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
      if(DEBUG == 1) Printf ("Stack space reservation failed\n");
      Exit ();
    }
    setStackSpace(pos - 1);
  }else{

    thread[head].clean = 0;
    if(DEBUG == 1) Printf("setting jump for thread %d\n", head);
    if (setjmp (thread[head].env) == 0) { /* save context of 1 */
    if(DEBUG == 1) Printf("setting env as %d\n", thread[head].env);
      longjmp (thread[current_tid].env, 1); /* back to thread 0 */
    }

    void (*f)() = thread[current_tid].func; /* f saves func on top of stack */
    int p = thread[current_tid].param;    /* p saves param on top of stack */
    if(DEBUG == 1) Printf("Executing thread %d program\n",MAXTHREADS - pos );
    /* here when thread 1 is scheduled for the first time */
    (*f) (p);     /* execute func (param) */

    MyExitThread ();    /* thread 1 is done - exit */

  }

}

void MyInitThreads ()
{
  int i;
  int setjum_ret;

  thread[0].valid = 1;      /* the initial thread is 0 */
  thread[0].clean = 1;
  for (i = 1; i < MAXTHREADS; i++) {  /* all other threads invalid */
    thread[i].valid = 0;
    thread[i].clean = 1;
  }

  MyInitThreadsCalled = 1;
  init_queue(&tid_queue);
  enqueue(&tid_queue, 0);

  char s[STACKSIZE];
  setjum_ret = setjmp(thread[0].clean_env);
  if(DEBUG == 1) Printf("thread %d env %d\n", 0, thread[0].clean_env);
  if( setjum_ret == 0){
    if (((int) &s[STACKSIZE-1]) - ((int) &s[0]) + 1 != STACKSIZE) {
      if(DEBUG == 1) Printf ("Stack space reservation failed\n");
      Exit ();
    }
    setStackSpace(MAXTHREADS - 1);
  }else if(setjum_ret == 2){
    if(DEBUG == 1) Printf("finish carving stack\n");
    return;
  }else{
    thread[0].clean = 0;
    if(DEBUG == 1) Printf("setting jump for thread %d\n", 0);
    if (setjmp (thread[0].env) == 0) { /* save context of 1 */
    if(DEBUG == 1) Printf("setting env as %d\n", thread[0].env);
      longjmp (thread[current_tid].env, 1); /* back to thread 0 */
    }

    void (*f)() = thread[current_tid].func; /* f saves func on top of stack */
    int p = thread[current_tid].param;    /* p saves param on top of stack */
    /* here when thread 1 is scheduled for the first time */
    (*f) (p);     /* execute func (param) */
    MyExitThread();
    return;
  }

}

/*  MySpawnThread (func, param) spawns a new thread to execute
 *  func (param), where func is a function with no return value and
 *  param is an integer parameter.  The new thread does not begin
 *  executing until another thread yields to it.
 */

int MySpawnThread (func, param)
  void (*func)();   /* function to be executed */
  int param;    /* integer parameter */
{
  if (! MyInitThreadsCalled) {
    Printf ("MySpawnThread: Must call MyInitThreads first\n");
    Exit ();
  }

  //have problem here, what if all 10 threads are running, and another one comes in
  for (int i = 0; i < MAXTHREADS; i++) {  /* all other threads invalid */
    if(thread[search_from].valid == 0){
      thread[search_from].valid = 1; /* mark the entry for the new thread valid */
      head = search_from;
      search_from = (search_from + 1) % MAXTHREADS;
      break;
    }
    search_from++;
    search_from = search_from % MAXTHREADS;
  }
  if(DEBUG == 1) Printf("%d enque\n", head);
  enqueue(&tid_queue, head);
  if (setjmp (thread[current_tid].env) == 0) {  /* save context of thread 0 */

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
    thread[head].func = func;
    thread[head].param = param;
    if(thread[head].clean == 1){
      if(DEBUG == 1) Printf("Setting thread %d function\n", head);
      longjmp(thread[head].clean_env, 1);
    }else{
      if(DEBUG == 1) Printf("Jump to excuting %d function\n", head);
      longjmp(thread[head].env, 1);
    }
  }

  return head;

  //what if maxthreads are all valid?

}

/*  MyYieldThread (t) causes the running thread to yield to thread t.
 *  Returns id of thread that yielded to t (i.e., the thread that called
 *  MyYieldThread), or -1 if t is an invalid id.
 */

int MyYieldThread (t)
  int t;        /* thread being yielded to */
{
  int parent_thread = current_tid;
  if (! MyInitThreadsCalled) {
    Printf ("MyYieldThread: Must call MyInitThreads first\n");
    Exit ();
  }
  if( t < 0 || t > MAXTHREADS){
    return (-1);
  }
  if (! thread[t].valid) {
    Printf ("MyYieldThread: Thread %d does not exist\n", t);
    return (-1);
  }

  if( current_tid == t){
    if(DEBUG == 1) Printf(" yield to your self\n");
    return current_tid;
  }
  if(DEBUG == 1) Printf("thread %d yield enqueue\n", current_tid);
  enqueue(&tid_queue, current_tid);
  //print_queue(&tid_queue);
  if(DEBUG == 1) Printf("thread %d yield dequeue\n", current_tid);
  dequeue(&tid_queue);
  //print_queue(&tid_queue);
  move_to_queue_head(&tid_queue, t);
  print_queue(&tid_queue);
  if(DEBUG == 1) Printf("rearrange thread %d to first\n", t);
  
  if (setjmp (thread[current_tid].env) == 0) {
    current_tid = t;
    if(DEBUG == 1) Printf("yield to thread %d long jump %d\n", t, thread[t].env);
    longjmp (thread[t].env, 1);
  }else{
    return parent_thread;
  }

}



/*  MyGetThread () returns id of currently running thread.
 */

int MyGetThread ()
{
  if (! MyInitThreadsCalled) {
    Printf ("MyGetThread: Must call MyInitThreads first\n");
    Exit ();
  }
  return current_tid;
}

/*  MySchedThread () causes the running thread to simply give up the
 *  CPU and allow another thread to be scheduled.  Selecting which
 *  thread to run is determined here.  Note that the same thread may
 *  be chosen (as will be the case if there are no other threads).
 */

void MySchedThread ()
{
  if (! MyInitThreadsCalled) {
    Printf ("MySchedThread: Must call MyInitThreads first\n");
    Exit ();
  }
  int tid;
  if(tid_queue.count > 0){
    if(thread[current_tid].valid == 1){
      enqueue(&tid_queue, current_tid);
    }
    if(DEBUG == 1) Printf("deque %d in sched thread\n", get_queue_first(&tid_queue));
    dequeue(&tid_queue);
    print_queue(&tid_queue);
    
    if(tid_queue.count == 0){
      Exit();
    }
    tid = get_queue_first(&tid_queue);
    if (setjmp (thread[current_tid].env) == 0) {
      current_tid = tid;
      longjmp(thread[tid].env, 1);
    }
  }else{
    Exit();
  }

}

/*  MyExitThread () causes the currently running thread to exit.
 */

void MyExitThread ()
{
  if (! MyInitThreadsCalled) {
    Printf ("MyExitThread: Must call MyInitThreads first\n");
    Exit ();
  }
  if(DEBUG == 1) Printf("thread %d exiting\n", current_tid);
  thread[current_tid].valid = 0;
  thread[current_tid].clean = 1;
  if(tid_queue.count > 0){
//    dequeue(&tid_queue);
    print_queue(&tid_queue);
    MySchedThread();
  }
  Exit();
}
