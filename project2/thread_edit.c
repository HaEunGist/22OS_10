/*Returns True when elm's priority is greater than e's priority*/
// thread.c
bool 
compare(struct list_elem *elm, struct list_elem *e, void *aux)		/*haeun*/
{
  if list_entry (elm, struct thread, elem)->priority > list_entry (e, struct thread, elem)->priority
  {
    return true;
  }
  return false;
}


void
thread_unblock (struct thread *t) // thread.c
{

  enum intr_level old_level; //original
  ASSERT (is_thread (t)); //original
  old_level = intr_disable (); //original
  ASSERT (t->status == THREAD_BLOCKED); //original

  struct thread *cur = running_thread(); //from schedule
  ASSERT (cur->status == THREAD_RUNNING); //from schedule
  if compare(&t->elem, &cur->elem, aux){ //steal running!
      t->status = THREAD_RUNNING; //like init_thread
      list_push_front(&ready_list, &cur->elem); //like thread_yield
      cur->status = THREAD_READY; //like thread_yield
      //pass scheduling current thread
  }
  else //~steal
  {
    list_insert_ordered (&ready_list, &cur->elem, compare, ); //original
    t->status = THREAD_READY; //original
  }

  intr_set_level (old_level);  //original

}


void
thread_yield (void)  // thread.c
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    list_insert_ordered (&ready_list, &t->elem, compare, );	/*haeun*/
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}