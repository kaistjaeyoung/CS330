#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <console.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
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
  switch(syscall_number) {
    case SYS_HALT:
      power_off();
      printf("SYS_HALT is called\n");
      break;
    case SYS_EXIT:
      exit(*(uint32_t *)(f->esp + 4));
      // thread_exit ();
      break;
    case SYS_EXEC:
      printf("SYS_EXEC is called\n");
      break;
    case SYS_WAIT:
      printf("SYS_WAIT is called\n");
      break;
    case SYS_CREATE:
      printf("SYS_CREATE is called\n");
      break;
    case SYS_REMOVE:
      printf("SYS_REMOVE is called\n");
      break;
    case SYS_OPEN:
      printf("SYS_OPEN is called\n");
      break;
    case SYS_FILESIZE:
      printf("SYS_FILESIZE is called\n");
      break;
    case SYS_READ:
      printf("SYS_READ is called\n");
      break;
    case SYS_WRITE:
      // hex_dump(f->esp, f->esp, 100, 1); 
      // printf("SYS_WRITE is called\n");   
      // f->eax = write((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
       f->eax = write((int)*(uint32_t *)(f->esp+4), (void *)*(uint32_t *)(f->esp + 8), (unsigned)*((uint32_t *)(f->esp + 12)));
      break;
    case SYS_SEEK:
      printf("SYS_SEEK is called\n");
      break;
    case SYS_TELL:
      printf("SYS_TELL is called\n");
      break;
    case SYS_CLOSE:
      printf("SYS_CLOSE is called\n");
      break;
    default:
      break;
  }

  // printf ("system call!\n");
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
  occurred. */

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
  // shutdown_power_off()
}

void exit (int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  /* Terminates the current user program, returning status to the kernel.
  If the process's parent waits for it (see below), this is the status that will be returned.
  Conventionally, a status of 0 indicates success and nonzero values indicate errors.*/
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
    // Fd 1 writes to the console. Your code to write to the console should write all of buffer in one call to putbuf()
    putbuf(buffer, size);
    // printf("size is? : %d\n", size);
    return size;
  }
  return -1; 
}

