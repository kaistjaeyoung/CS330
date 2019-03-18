#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <console.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"

static void syscall_handler (struct intr_frame *);

void halt (void);
void exit (int status);
int write (int fd, const void *buffer, unsigned size);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int syscall_number = *(int *)(f->esp);
  int fd = *(int *)(f->esp + 4);
  int buffer = *(int *)(f->esp + 8);
  int size = *(int *)(f->esp + 12);

  switch(syscall_number) {
    case SYS_HALT:
      halt ();
      break;
    case SYS_EXIT:
      is_valid_addr(f->esp + 4);
      exit(fd);
      break;
    case SYS_EXEC:
      is_valid_addr(f->esp + 4);
      f->eax = exec((const char *)fd);
      break;
    case SYS_WAIT:
      is_valid_addr(f->esp + 4);
      f->eax = wait((pid_t)fd);
      break; 
    case SYS_CREATE:
      f->eax = create((const char *)fd, (unsigned) buffer);
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
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);        
      is_valid_addr(f->esp + 12);        
      f->eax = write((int)fd, (void *)buffer, (unsigned)size);
      break;
    case SYS_SEEK:
      break;
    case SYS_TELL:
      break;
    case SYS_CLOSE:
      break;
    default:
      break;
  }
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
  occurred. */

void is_valid_addr(void *uaddr)
{
  if(!is_user_vaddr(uaddr)) exit (-1);
}

static int
get_user (const uint8_t *uaddr)
{
  // if (! ((void*)uaddr < PHYS_BASE)) {
  //   return -1;
  // }
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

void halt(void) 
{
  power_off();
}

void exit (int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  /* Terminates the current user program, returning status to the kernel.
  If the process's parent waits for it (see below), this is the status that will be returned.
  Conventionally, a status of 0 indicates success and nonzero values indicate errors.*/
  thread_current() -> exit_status = status; // ( by jy )
  thread_exit();
}

pid_t exec (const char *cmd_line)
{
  return process_execute(cmd_line);
}

int wait (pid_t pid)
{
  return process_wait(pid);
}

int read (int fd, void* buffer, unsigned size)
{
  int i;
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  }
  return i;
}


int write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    /* Fd 1 writes to the console. Your code to write to the console 
    should write all of buffer in one call to putbuf()*/
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

bool create (const char *file, unsigned initial_size)
{
  if (strcmp(file, "") == 0) return false;
  if (file == NULL) exit(-1);
  return filesys_create(file, initial_size);
}
