#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <console.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"
#include "threads/malloc.h"
#include <list.h>
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

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

static struct lock syscall_lock;

static struct fd
{
  int fd_value;
  struct list_elem fd_elem;
  struct file * file;
};

void
syscall_init (void) 
{
  lock_init (&syscall_lock);
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
      lock_acquire(&syscall_lock);
      f->eax = exec((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_WAIT:
      is_valid_addr(f->esp + 4);
      f->eax = wait((pid_t)fd);
      break; 
    case SYS_CREATE:
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);      
      lock_acquire(&syscall_lock);
      f->eax = create((const char *)fd, (unsigned) buffer);
      lock_release(&syscall_lock);
      break;
    case SYS_REMOVE:
      is_valid_addr(f->esp + 4);
      lock_acquire(&syscall_lock);
      f->eax = remove((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_OPEN:
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);        
      is_valid_addr(f->esp + 12);
      lock_acquire(&syscall_lock);
      f->eax = open((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_FILESIZE:
      is_valid_addr(f->esp + 4);
      f->eax = filesize ((int)fd);
      break;
    case SYS_READ:
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);        
      is_valid_addr(f->esp + 12);
      lock_acquire(&syscall_lock);
      f->eax = read((int)fd, (void *)buffer, (unsigned)size);
      lock_release(&syscall_lock);
      break;
    case SYS_WRITE:
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);        
      is_valid_addr(f->esp + 12);
      lock_acquire(&syscall_lock);  
      f->eax = write((int)fd, (void *)buffer, (unsigned)size);
      lock_release(&syscall_lock);
      break;
    case SYS_SEEK:
      is_valid_addr(f->esp + 4);        
      is_valid_addr(f->esp + 8);
      seek((int)fd, (unsigned)buffer);
      break;
    case SYS_TELL:
      printf("call tell\n");
      break;
    case SYS_CLOSE:
      is_valid_addr(f->esp + 4);        
      close((int)fd);
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
      ((char *)buffer)[i] = input_getc();
      if (((char *)buffer)[i] == '\0') {
        break;
      }
    }
  return i;
  } else {
  struct thread * curr;
  struct list_elem * e;
  off_t reading_size = 0;
  
  curr = thread_current ();
  for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
    {
      if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {
        reading_size = file_read(list_entry(e, struct fd, fd_elem)->file, buffer, size);
      }
    }
    return reading_size;
  }
}

int write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    /* Fd 1 writes to the console. Your code to write to the console 
    should write all of buffer in one call to putbuf()*/
    putbuf(buffer, size);
    return size;
  } else {
    struct thread * curr;
    struct list_elem * e;
    off_t write_size = 0;
  
    curr = thread_current ();
    for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
      {
        if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {
          // by jy
          if ( strcmp(thread_current()->name, list_entry(e, struct fd, fd_elem))==0 ) {
            file_deny_write(list_entry(e, struct fd, fd_elem)->file);
          }
          
          write_size = file_write(list_entry(e, struct fd, fd_elem)->file, buffer, size);
        }
      }
      return write_size;
  }
  return -1; 
}

bool create (const char *file, unsigned initial_size)
{
  if (strcmp(file, "") == 0) return false;
  if (file == NULL) exit(-1);
  return filesys_create(file, initial_size);
}

int open (const char *file)
{
  if (file == NULL) exit(-1);
  struct file * openfile = filesys_open(file);
  if (openfile == NULL) return -1;

  struct fd  * new_fd = (struct fd*) malloc(sizeof(struct fd));
  new_fd->file = openfile;

  // if the file is on running process, deny write (by jy)
  // if ( strcmp(thread_current()->name, file)==0 ) {
  //   file_deny_write(openfile);
  // }
  
  if (!list_empty(&thread_current ()->fd_list)) {
    new_fd->fd_value = thread_current () -> max_fd++;
    list_push_back(&thread_current ()->fd_list, &new_fd->fd_elem);
    return new_fd->fd_value;
  } else {
    new_fd->fd_value = 3;
    thread_current() -> max_fd = 3;
    list_push_back(&thread_current ()->fd_list, &new_fd->fd_elem);
    return 3;
  }
}

int filesize (int fd)
{
  struct thread * curr;
  struct list_elem * e;
  off_t file_size = 0;
  curr = thread_current ();
  for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
    {
      if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {
        file_size = file_length(list_entry(e, struct fd, fd_elem)->file);
      }
    }
  return file_size;
}

void close (int fd)
{
  struct thread * curr;
  struct list_elem * e;
  curr = thread_current ();
  for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
    {
      if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {
        close(list_entry(e, struct fd, fd_elem)->file);
      }
    }
}

bool remove (const char *file)
{
  return filesys_remove(file);
}

void seek (int fd, unsigned position)
{
  struct thread * curr;
  struct list_elem * e;

  curr = thread_current ();
  for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
    {
      if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {
        file_seek(list_entry(e, struct fd, fd_elem)->file, position);
      }
    }
}