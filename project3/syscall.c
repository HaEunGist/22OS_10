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
#include "filesys/off_t.h" //proj3


struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };
struct lock filesys_lock;

static void syscall_handler (struct intr_frame *);
void check_type(const void *addr); //proj3

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock); //proj3
}

//proj3
static void
syscall_handler (struct intr_frame *f) 
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
      f->eax = wait ((pid_t)*(uint32_t *)(f->esp + 4)); //return exist
      break;
    case SYS_CREATE:
      check_type(f->esp + 4); //16
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
      f->eax = filesize((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_READ:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      check_type(f->esp + 12);
      f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_WRITE:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      check_type(f->esp + 12);
      f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_SEEK:
      check_type(f->esp + 4);
      check_type(f->esp + 8);
      seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 12));
      break;
    case SYS_TELL:
      check_type(f->esp + 4);
      f->eax = tell((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CLOSE:
      check_type(f->esp + 4);
      close((int)*(uint32_t *)(f->esp + 4));
      break;
    default:
      exit(-1);
  }
  //printf ("system call!\n");
  //thread_exit ();
}


//proj3
void check_type(const void *addr)
{ 
  // threads/vaddr.h
  if (!is_user_vaddr(addr)){
    exit(-1);
  }
}

/* proj3 - system call implementation */
void halt (void) {
  shutdown_power_off();
}
void exit (int status) {
  int i;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current()->exit_status = status;
  for (i = 3; i < 128; i++) {
      if (thread_current()->fdt[i] != NULL) {
          close(i);
      }   
  }   
  thread_exit (); 
}

pid_t exec (const char *cmd_line) {
  return process_execute(cmd_line);
}

int wait (pid_t pid) {
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size) {
  if (file == NULL) {
      exit(-1);
  }
  check_type(file);
  return filesys_create(file, initial_size);

}

bool remove (const char *file) {
  if (file == NULL) {
      exit(-1);
  }   
  check_type(file);
  return filesys_remove(file);
}

int open (const char *file) {
  int i;
  int ret = -1;
  struct file* fp;
  if (file == NULL) {
      exit(-1);
  }
  check_type(file);
  lock_acquire(&filesys_lock);
  fp = filesys_open(file);
  if (fp == NULL) {
      ret = -1;
  } else {
    for (i = 3; i < 128; i++) {
      if (thread_current()->fdt[i] == NULL) {
        if (strcmp(thread_current()->name, file) == 0) {
            file_deny_write(fp);
        }
        thread_current()->fdt[i] = fp;
        ret = i;
        break;
      }
    }
  }
  lock_release(&filesys_lock);
  return ret;
}

int filesize (int fd) {
  if (thread_current()->fdt[fd] == NULL) {
      exit(-1);
  }
  return file_length(thread_current()->fdt[fd]);
}

int read (int fd, void* buffer, unsigned size) {
  int i;
  int ret;
  check_type(buffer);
  lock_acquire(&filesys_lock);
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
    ret = i;
  } else if (fd > 2) {
    if (thread_current()->fdt[fd] == NULL) {
      exit(-1);
    }
    ret = file_read(thread_current()->fdt[fd], buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}

int write (int fd, const void *buffer, unsigned size) {

  int ret = -1;
  check_type(buffer);
  lock_acquire(&filesys_lock);
  if (fd == 1) {
    putbuf(buffer, size);
    ret = size;
  } else if (fd > 2) {
    if (thread_current()->fdt[fd] == NULL) {
      lock_release(&filesys_lock);
      exit(-1);
    }
    if (thread_current()->fdt[fd]->deny_write) {
        file_deny_write(thread_current()->fdt[fd]);
    }
    ret = file_write(thread_current()->fdt[fd], buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}
void seek (int fd, unsigned position) {
  if (thread_current()->fdt[fd] == NULL) {
    exit(-1);
  }
  file_seek(thread_current()->fdt[fd], position);
}

unsigned tell (int fd) {
  if (thread_current()->fdt[fd] == NULL) {
    exit(-1);
  }
  return file_tell(thread_current()->fdt[fd]);
}

void close (int fd) {
  struct file* fp;
  if (thread_current()->fdt[fd] == NULL) {
    exit(-1);
  }
  fp = thread_current()->fdt[fd];
  thread_current()->fdt[fd] = NULL;
  return file_close(fp);
} 