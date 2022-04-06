//-----------------------1---------------------//

/*Returns True when elm's priority is greater than e's priority*/
// thread.c
bool //fin
compare(const struct list_elem *elm, const struct list_elem *e, void *aux UNUSED)		/*haeun*/
{
  if (list_entry (elm, struct thread, elem)->priority > list_entry (e, struct thread, elem)->priority)
  {
    return true;
  }
  return false;
}

//** compare thread.h에 추가

void //fin
thread_unblock (struct thread *t) // thread.c
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  list_insert_ordered (&ready_list, &t->elem, compare, NULL);	/*haeun*/
  t->status = THREAD_READY;
  intr_set_level (old_level);
}

void //fin
thread_yield (void)  // thread.c
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    list_insert_ordered (&ready_list, &cur->elem, compare, NULL);	/*haeun*/
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

void
thread_set_priority (int new_priority) //커런트 스레드의 우선순위 변경하는 상황
{ 
  thread_current ()->priority = new_priority;
  
  struct thread *cur = running_thread ();
  

  if ( !list_empty (&ready_list)){
    struct thread *next = list_entry (list_begin(&ready_list), struct thread, elem);
    if (compare(&next->elem, &cur->elem, NULL)) //if next has higher priority
    {
    thread_yield(); //next가 더 크면 러닝 멈추기
    }
  }
}




int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  enum intr_level old_level;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Prepare thread for first run by initializing its stack.
     Do this atomically so intermediate values for the 'stack' 
     member cannot be observed. */
  old_level = intr_disable ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  intr_set_level (old_level);

  /* Add to run queue. */
  thread_unblock (t);

//edit
//thread_current's priority vs priority랑 비교
  if (priority > thread_current ()->priority){
    thread_yield();
  }
// => thread_yield();

  return tid;
}


static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    list_sort (&ready_list, compare, NULL); //0406
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

//-----------------------2---------------------//


void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) 
    list_sort (&sema->waiters, compare, NULL); //0406
    thread_unblock (list_entry (list_pop_front (&sema->waiters),
                                struct thread, elem));
  sema->value++;
  intr_set_level (old_level);
}

void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
      list_insert_ordered (&sema->waiters, &thread_current ()->elem, compare, NULL); //0406
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);
}

void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  //---
  //int cur_priority = thread_get_priority ();
  struct thread *hold = lock->holder;
  //struct list donating = thread_current()->donating_list;
/*
  list_push_back (&donating, &hold->elem);
  if(!list_empty (&hold->donating_list)){

  }
*/
  hold->origin_priority = hold->priority;
  hold->priority = thread_get_priority ();

  //&thread_current()->origin_priority = &thread_current ()->priority;        //---haeun2---//

  sema_down (&lock->semaphore);
  lock->holder = thread_current ();
}


void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  struct thread *hold = lock->holder;

  hold = NULL;
  sema_up (&lock->semaphore);

  thread_set_priority (hold->origin_priority);
}











//-----------------trash------------------------//
void
thread_unblock (struct thread *t) // thread.c //simultaneus
{

  enum intr_level old_level; //original
  ASSERT (is_thread (t)); //original
  old_level = intr_disable (); //original
  ASSERT (t->status == THREAD_BLOCKED); //original

  struct thread *cur = running_thread(); //from schedule
  //ASSERT (cur->status == THREAD_RUNNING); //from schedule
  if (compare(&t->elem, &cur->elem, NULL)){ //steal running!
      t->status = THREAD_RUNNING; //like init_thread
      list_push_front(&ready_list, &cur->elem); //like thread_yield
      cur->status = THREAD_READY; //like thread_yield
      struct thread *prev = switch_threads(cur, t);//0402 edit
      //pass scheduling current thread
  }
  else //~steal
  {
    list_insert_ordered (&ready_list, &t->elem, compare, NULL); //original
    t->status = THREAD_READY; //original
  }

  intr_set_level (old_level);  //original

}

void
donate_to_holders(){
  /*
  H의 입장에서,
  M을 H의 donating list에 추가
  만약 M의 donating에 원소들이 있다면
  그 원소들도 모두 추가
  &
  donating list의 모든 원소들의 priority 변경*/
}