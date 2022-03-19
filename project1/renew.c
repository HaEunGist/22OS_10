// 수정하고 나면 haeun 라고 태그 달기

// 1. sleeping 된 함수들을 일단 block시키기



../../threads/thread.c:97: error: ‘sleeping_list’ undeclared (first use in this function)
../../threads/thread.c:97: error: (Each undeclared identifier is reported only once
../../threads/thread.c:97: error: for each function it appears in.)



/* 여기는 sleeping_ticks 추가 */ //fin //code
// =---------------thread.h------------------//
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

    int64_t sleeping_ticks; /* haeun */    

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

/* 파일 열어서 선언한 곳에 sleeping_list 추가로 선언해두기 */ //fin //code
// =---------------thread.c------------------//
/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;
static struct list sleeping_list; /* haeun */

/* sleeping_list 초기화도 해야함 */ //fin //code
// =---------------thread.c------------------//
void
thread_init (void)
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);
  list_init (&sleeping_list); /* haeun */

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* timer_sleep 함수가 호출되면 일단 block하는 형태로 수정*/ //fin 
// =---------------timer.c------------------//
void
timer_sleep (int64_t ticks)
{
  int64_t start = timer_ticks ();
  ASSERT (intr_get_level () == INTR_ON);

  enum intr_level old_level;                /* haeun */
  //intr off  before thread
  old_level = intr_disable ();              /* haeun */

  //sleeping_ticks setting (to start+ticks)
  //thread into sleeping_list
  thread_toSleep(start+ticks);              /* haeun */

  thread_block();                           /* haeun */
  intr_set_level (old_level);               /* haeun */
}

// =---------------thread.c------------------// //code
void
thread_toSleep(int64_t ticks){              /* haeun */

    struct thread *cur = thread_current();
    cur->sleeping_ticks = ticks //**

    list_push_back(&sleeping_list, &cur->elem);

}


// 2. interrupt가 발생했을 때, 필요한 thread 깨우기

/* timer_interrupt에서 깨우는 함수 호출 */
// =---------------timer.c------------------//
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  thread_awake();               /* haeun */
}

/* 시간 된 thread 깨우기 */
// =---------------thread.c------------------// //code
void
thread_awake(int64_t ticks){                /* haeun */

    // checking sleep_list
    struct list_elem *e = list_begin(&sleeping_list);
    while(e != list_end(&sleeping_list)){
        struct thread *t = list_entry(e, struct threda, elem); //**
        // if sleeping_time ends
        if (t->sleeping_ticks <= ticks){
            // remove from sleeping_list
            e = list_remove(e); //**
            // unblock
            thread_unblock(t);
        } else {
            e = list_next(e); //**
        }
        
    }

}







/**/
/**/
/**/
/**/


/* thread를 sleeping list에 푸쉬하는 함수는 원래 yeild, unblock에만 있어서
timer_sleep 에서 block이랑 sleeping 푸쉬를 순차적으로 진행해야함*/




