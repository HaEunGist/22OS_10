  /*
  1. struct intr_frame *f UNUSED 으로부터 stack pointer를 get
  2. stack pointer 로부터 system call number를 get
  2-1. system call number != user 영역
      -> 프로세스 종료
  2-2. 
  3. system call number가 exit면 -> exit 시스템 콜 실행 [switch]
    [exit 시스템 콜]
  */
  /*
  1. intr_frame -> get 유저스택포인터(esp)주소
    - 인자가 가리키는 주소(포인터)가 유저영역인지 확인 [check_address]
      - 유저영역이 아니면 프로세스 종료 [exit(-1)]
  2. 유저 스택에 있는 인자들을 커널에 저장 [get_argument]
    - 스택(esp)에서 인자들을 4byte 크기로 꺼내서, 인자의 주소값을, arg에 순차적으로 저장
    - count 개수만큼의 인자를 스택에서 가져옴
    - 인자들이 저장된 위치가 유저영역인지 확인
    * f->esp의 첫 4바이트 = 시스템콜넘버, 그 다음 4바이트씩 count 횟수만큼 긁어와서 arg에 저장
  4. 시스템 콜 함수의 리턴값은 프레임의 eax에 저장
  */


/*-----------------proj3_1-----------------*/
// userprog/syscall.c
static void
syscall_handler (struct intr_frame *f UNUSED) //proj3
{
  switch(*(uint32_t *)(f->esp)){
    case SYS_HALT:
      break;

    case SYS_EXIT: //lib/syscall_nr.h
      check_type(f->esp + 4);
      exit(*(uint32_t *)(f->esp + 4));
      break;

    case SYS_EXEC:
      break;
    case SYS_WAIT:
      break;
    case SYS_CREATE:
      break;
    case SYS_REMOVE:
      break;
    case SYS_OPEN:
      break;
    case SYS_FILESIZE:
      break;
    case SYS_READ:
      break;
    case SYS_WRITE:
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;

    default:
      exit(-1);
      break;
  }

  //printf ("system call!\n");
  //thread_exit ();
}

// userprog/syscall.c
void check_type(void *addr){ //proj3
  if (!is_user_vaddr(addr)){ // threads/vaddr.h
    exit(-1);
  }
}



/*-----------------proj3_3-----------------*/
void halt (void) {
  shutdown_power_off(); //in devices/shutdown.h
}
void exit(int status){ //proj3
  printf("%s: exit(%d)\n", thread_name(), status);
  // 스레드 종료 status를 저장해야하나 ..?
  thread_exit();
};
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);