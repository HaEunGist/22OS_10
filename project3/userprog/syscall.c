#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h" //proj3
#include "threads/vaddr.h" //proj3
#include "lib/user/syscall.h" //proj3
#include "devices/input.h" //proj3
#include "threads/synch.h" //proj3
#include "filesys/file.h" //proj3
#include "filesys/filesys.h" //proj3
#include "userprog/process.h" //proj3

static void syscall_handler (struct intr_frame *);
void check_type(void *addr); //proj3

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(filesys_lock); //proj3
}

//proj3
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch(*(uint32_t *)(f->esp)){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      check_type(f->esp + 4);
      exit(*(uint32_t *)(f->esp + 4));
      break;
    case SYS_EXEC:
      check_type(f->esp + 4);
      f->eax = exec ((const char *)*(uint32_t *)(f->esp + 4)); //return exist
      break;
    case SYS_WAIT:
      check_type(f->esp + 4);
      f->eax = wait ((pid_t)(uint32_t *)(f->esp + 4)); //return exist
      break;
    case SYS_CREATE:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      f->eax = create ((const char *)*(uint32_t *)(f->esp + 4),
                        *(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      check_type(f->esp + 4);
      f->eax = remove ((const char *)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_OPEN:
      check_type(f->esp + 4);
      f->eax = open ((const char *)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_FILESIZE:
      check_type(f->esp + 4);
      f->eax = filesize (*(uint32_t *)(f->esp + 4));
      break;
    case SYS_READ:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      check_type(f->esp + 12);
      f->eax = read (*(uint32_t *)(f->esp + 4),
                      (void *)(uint32_t *)(f->esp + 8),
                      *(uint32_t *)(f->esp + 12));
      break;
    case SYS_WRITE:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      check_type(f->esp + 12);
      f->eax = write (*(uint32_t *)(f->esp + 4),
                      (const void *)(uint32_t *)(f->esp + 8),
                      *(uint32_t *)(f->esp + 12));
      break;
    case SYS_SEEK:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      seek (*(uint32_t *)(f->esp + 4),
            *(uint32_t *)(f->esp + 8));
      break;
    case SYS_TELL:
      check_type(f->esp + 4);
      f->eax = tell (*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CLOSE:
      check_type(f->esp + 4);
      close (*(uint32_t *)(f->esp + 4));
      break;
    default:
      exit(-1);
      break;
  }
  //printf ("system call!\n");
  //thread_exit ();
}

//proj3
void check_type(void *addr)
{ 
  if (!is_user_vaddr(addr)){
    exit(-1);
  }
}

/* proj3 - system call implementation */
void halt (void) {
  shutdown_power_off();
}
void exit(int status){ 
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  thread_exit();
}
pid_t exec (const char *file)
{ 
  int tid = process_execute(file); //자식 프로세스 생성
  struct list_elem* e;
  for (e=list_begin(&(thread_current()->children)); e!=list_end(&(thread_current()->children)); e=list_next(e)){
    struct thread* tmp = list_entry(e, struct thread, children_elem);
    if (tid == tmp->tid){
      sema_down(&(thread_current()->sema_load));
      list_remove(&(tmp->children_elem));
      return tid;
    }
  }
  return -1;
}
int wait (pid_t pid)
{
  return process_wait(pid);
}
bool create (const char *file, unsigned initial_size){
  return filesys_create(file, initial_size);
}
bool remove (const char *file){
  return filesys_remove(file);
}
int open (const char *file)
{
  struct file * tmp = filesys_open(file);
  if (tmp==NULL){
    return -1;
  } else {
    while(thread_current()->fdt_can_use > 2 && thread_current()->fdt_can_use<=sizeof(thread_current()->fdt)){
      if (thread_current()->fdt[thread_current()->fdt_can_use] == NULL){
        thread_current()->fdt[thread_current()->fdt_can_use] = tmp;
        return thread_current()->fdt_can_use;
      } else {
        thread_current()->fdt_can_use++;
      }
    }
    return NULL;
  } 
}
int filesize (int fd) 
{
  struct file * tmp = thread_current()->fdt[fd];
  if (tmp==NULL){
    return -1;
  } else {
    return file_length (tmp); 
  }
}
int read (int fd, void *buffer, unsigned size)
{
  lock_acquire (filesys_lock);
  if (fd==0){
    *(char *)buffer = input_getc();
    lock_release (filesys_lock);
    return size;
  } else {
    struct file * tmp = thread_current()->fdt[fd];
    int result = file_read (tmp, buffer, size); 
    lock_release (filesys_lock);
    return result;
  }
}
int write (int fd, const void *buffer, unsigned size)
{
  lock_acquire (filesys_lock);
  if (fd==1){
    putbuf(buffer, size);
    lock_release (filesys_lock);
    return size;
  } else {
    struct file * tmp = thread_current()->fdt[fd];
    int result = file_write (tmp, buffer, size) ;
    lock_release (filesys_lock);
    return result;
  }
}
void seek (int fd, unsigned position) 
{
  struct file * tmp = thread_current()->fdt[fd];
  file_seek (tmp, position);
}
unsigned tell (int fd) 
{
  struct file * tmp = thread_current()->fdt[fd];
  return file_tell (tmp);
}
void close (int fd)
{
  struct file * tmp = thread_current()->fdt[fd];
  file_close (tmp); 
  thread_current()->fdt[fd] = NULL;
  thread_current()->fdt_can_use--;
}
