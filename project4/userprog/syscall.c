
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/off_t.h"
#include "vm/page.h"

struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

static void syscall_handler (struct intr_frame *);
struct vm_entry * check_user_vaddr(const void *vaddr, void *esp);
struct lock filesys_lock;

struct vm_entry * check_user_vaddr(const void *vaddr, void *esp) {
  if (!is_user_vaddr(vaddr)) {
    exit(-1);
  }

  /*add이 vm_entry에 존재하면 vm_entry 반환 */
  /*
  struct vm_entry *vme = find_vme(vaddr); //find_vme 함수 구현
  if (vme != NULL){
    return vme;
  }
  else {
    if (!verify_stack(vaddr, esp)){ //NEED FIX, verify_stack 함수 구현?
      exit(-1); 
    }
    expand_stack(vaddr);  //expand_stack 함수 구현?
    vme = find_vme(vaddr);
    return vme;
  }*/
  return find_vme(vaddr);
}

//buffer의 유효성을 검사하는 함수
void check_valid_buffer(void *buffer, unsigned size, void *esp, bool to_write)
{
	void *ptr=pg_round_down(buffer);
	//인자로 받은 buffer로부터 buffer + size까지의 크기가 한 페이지의 크기를 넘길 수 도 있음
	for(;ptr<buffer+size; ptr+=PGSIZE)
	{
		//check_address를 이용해서 주소의 유저영역 여부를 검사함과 동시에 vm_entry 구조체를 얻음
		struct vm_entry * vme = check_user_vaddr(ptr, esp);
		//해당 주소에 대한 vm_entry의 존재 여부와 vm_entry의 writable 멤버가 true인지 검사
		if(vme == NULL || !(vme->writable))
		{
			exit(-1);
		}
		//위 내용을 buffer부터 buffer+size까지의 주소에 포함되는 vm_entry들에 대해 적용

	}


}

void
syscall_init (void) 
{
  lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int len;
  check_user_vaddr(f->esp, f->esp);
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      check_user_vaddr(f->esp + 4, f->esp);
      exit(*(uint32_t *)(f->esp + 4));
      break;
    case SYS_EXEC:
      check_user_vaddr(f->esp + 4, f->esp);
      len = 0;
      while((((char *)(uint32_t *)(f->esp + 4))[len] != '\0')){
        len ++;
      }
      check_str((void *)*(uint32_t *)(f->esp + 4), len, f->esp); //esp+4??
      f->eax = exec((const char *)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_WAIT:
      f-> eax = wait((pid_t)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CREATE:
      check_user_vaddr(f->esp + 4, f->esp);
      check_user_vaddr(f->esp + 8, f->esp);
      f->eax = create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_REMOVE:
      check_user_vaddr(f->esp + 4, f->esp);
      f->eax = remove((const char*)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_OPEN:
      check_user_vaddr(f->esp + 4, f->esp);
      len = 0;
      while((((char *)*(uint32_t *)(f->esp + 4))[len] != '\0')){
        len ++;
      }
      check_str((void *)*(uint32_t *)(f->esp + 4), len, f->esp); //esp+4??
      f->eax = open((const char*)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_FILESIZE:
      check_user_vaddr(f->esp + 4, f->esp);
      f->eax = filesize((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_READ:
      check_user_vaddr(f->esp + 4, f->esp);
      check_user_vaddr(f->esp + 8, f->esp);
      check_user_vaddr(f->esp + 12, f->esp);
      check_valid_buffer((void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)), f -> esp, true);
      f->eax = read((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_WRITE:
      check_valid_buffer((void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)), f -> esp, true);
      f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_SEEK:
      check_user_vaddr(f->esp + 4, f->esp);
      check_user_vaddr(f->esp + 8, f->esp);
      seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
      break;
    case SYS_TELL:
      check_user_vaddr(f->esp + 4, f->esp);
      f->eax = tell((int)*(uint32_t *)(f->esp + 4));
      break;
    case SYS_CLOSE:
      check_user_vaddr(f->esp + 4, f->esp);
      close((int)*(uint32_t *)(f->esp + 4));
      break;
  }
}

void halt (void) {
  shutdown_power_off();
}

void exit (int status) {
  int i;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_current() -> exit_status = status;
  for (i = 3; i < 128; i++) {
      if (thread_current()->fd[i] != NULL) {
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

int filesize (int fd) {
  if (thread_current()->fd[fd] == NULL) {
      exit(-1);
  }
  return file_length(thread_current()->fd[fd]);
}

int read (int fd, void* buffer, unsigned size) {
  int i;
  int ret;
  //check_user_vaddr(buffer);
  lock_acquire(&filesys_lock);
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
    ret = i;
  } else if (fd > 2) {
    if (thread_current()->fd[fd] == NULL) {
      exit(-1);
    }
    ret = file_read(thread_current()->fd[fd], buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}

int write (int fd, const void *buffer, unsigned size) {

  int ret = -1;
  //check_user_vaddr(buffer);
  lock_acquire(&filesys_lock);
  if (fd == 1) {
    putbuf(buffer, size);
    ret = size;
  } else if (fd > 2) {
    if (thread_current()->fd[fd] == NULL) {
      lock_release(&filesys_lock);
      exit(-1);
    }
    if (thread_current()->fd[fd]->deny_write) {
        file_deny_write(thread_current()->fd[fd]);
    }
    ret = file_write(thread_current()->fd[fd], buffer, size);
  }
  lock_release(&filesys_lock);
  return ret;
}

bool create (const char *file, unsigned initial_size) {
  if (file == NULL) {
      exit(-1);
  }
  //check_user_vaddr(file);
  return filesys_create(file, initial_size);
}

bool remove (const char *file) {
  if (file == NULL) {
      exit(-1);
  }
  //check_user_vaddr(file);
  return filesys_remove(file);
}

int open (const char *file) {
  int i;
  int ret = -1;
  struct file* fp;
  if (file == NULL) {
      exit(-1);
  }
  //check_user_vaddr(file);
  lock_acquire(&filesys_lock);
  fp = filesys_open(file);
  if (fp == NULL) {
      ret = -1;
  } else {
    for (i = 3; i < 128; i++) {
      if (thread_current()->fd[i] == NULL) {
        if (strcmp(thread_current()->name, file) == 0) {
            file_deny_write(fp);
        }
        thread_current()->fd[i] = fp;
        ret = i;
        break;
      }
    }
  }
  lock_release(&filesys_lock);
  return ret;
}

void seek (int fd, unsigned position) {
  if (thread_current()->fd[fd] == NULL) {
    exit(-1);
  }
  file_seek(thread_current()->fd[fd], position);
}

unsigned tell (int fd) {
  if (thread_current()->fd[fd] == NULL) {
    exit(-1);
  }
  return file_tell(thread_current()->fd[fd]);
}

void close (int fd) {
  struct file* fp;
  if (thread_current()->fd[fd] == NULL) {
    exit(-1);
  }
  fp = thread_current()->fd[fd];
  thread_current()->fd[fd] = NULL;
  return file_close(fp);
}

/* 인자의 string의 주소가 유효한 가상주소인지 검사 */
void check_str (const void *str, unsigned len, void *esp){
  void *ptr = pg_round_down(str);
  for (; ptr< str + len; ptr += PGSIZE){    //COPY, NEED FIX
    if (check_user_vaddr(ptr, esp) == NULL || !(check_user_vaddr(ptr, esp)->writable)){    //struct * vme = check_user_vaddr(ptr, esp); 였는데 * 필요할 수도 있음..
      exit(-1);
    }
  }
}

/*
// system call에서 사용할 인자의 문자열의 주소값이 유효한 가상 주소인지 검사하는 함수로 null문자를 이용하는 것이 아닌 사이즈를 이용
void
check_valid_string_length (void *str, unsigned size, void *esp) //NEED FIX
{
	int i;
	for(i=0; i<size; i++)
	{
		struct vm_entry *vme = check_address ((void *) (str++), esp);
		if(vme == NULL)
			exit(-1);
	}
}*/


/*
//proj4
int mmap(int fd, void* addr){
    struct file file = process_get_file(fd);
    if (file==NULL){
        return exit(-1);
    } else {
        file_reopen(file);
        //mapid 할당
        //mmap_file 생성 및 초기화
        //vm_entry 생성 및 초기화
        return fd;
    }
}

void munmap(int mapping){
    struct list_elem *e;
    struct thread *cur = thread_current();
    if (mapping==CLOSE_ALL){
        //모든 파일 매핑 제거
    }
    for (e = list_begin (&(cur->mmap_list)); e != list_end (&all_list);
       e = list_next (e)){
           //mapping == mapid인 모든 vm_entry 제거
           //페이지 테이블 엔트리 제거
           // mmap_file 제거
           //file_close
       }
    
}
*/