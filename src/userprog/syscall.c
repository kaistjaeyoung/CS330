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
#include "vm/page.h"

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
struct fd * find_fd(struct thread * curr, int fd);
mapid_t mmap (int fd, void *addr);
void munmap (mapid_t mapping);

static struct lock syscall_lock;

static struct fd
{
  int fd_value;
  struct list_elem fd_elem;
  struct file * file;
};

static struct mmap_list_entry
{
  mapid_t mapid;
  uint32_t* user_vaddr;
  struct file * file;
  int offset;
  struct list_elem elem;   
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

  is_valid_addr(f->esp + 4);
  is_valid_addr(f->esp + 8);   
  is_valid_addr(f->esp + 12);         

  switch(syscall_number) {
    case SYS_HALT:
      halt ();
      break;
    case SYS_EXIT:
      exit(fd);
      break;
    case SYS_EXEC:
      lock_acquire(&syscall_lock);
      f->eax = exec((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_WAIT:
      f->eax = wait((pid_t)fd);
      break; 
    case SYS_CREATE:
      lock_acquire(&syscall_lock);
      f->eax = create((const char *)fd, (unsigned) buffer);
      lock_release(&syscall_lock);
      break;
    case SYS_REMOVE:
      lock_acquire(&syscall_lock);
      f->eax = remove((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_OPEN:
      lock_acquire(&syscall_lock);
      f->eax = open((const char *)fd);
      lock_release(&syscall_lock);
      break;
    case SYS_FILESIZE:
      f->eax = filesize ((int)fd);
      break;
    case SYS_READ:
      lock_acquire(&syscall_lock);
      f->eax = read((int)fd, (void *)buffer, (unsigned)size);
      lock_release(&syscall_lock);
      break;
    case SYS_WRITE:
      lock_acquire(&syscall_lock);  
      f->eax = write((int)fd, (void *)buffer, (unsigned)size);
      lock_release(&syscall_lock);
      break;
    case SYS_SEEK:
      seek((int)fd, (unsigned)buffer);
      break;
    case SYS_TELL:
      printf("call tell\n");
      break;
    case SYS_CLOSE:
      is_valid_addr(f->esp + 4);     
      close((int)fd);
      break;
    case SYS_MMAP:
      lock_acquire(&syscall_lock);  
      f->eax = mmap((int)fd, (void *)buffer);
      lock_release(&syscall_lock);  
      break;
    case SYS_MUNMAP:
      // lock_acquire(&syscall_lock);  
      munmap ((mapid_t) fd);
      // lock_release(&syscall_lock);  
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

  struct thread * curr;
  struct list_elem * e;
  curr = thread_current ();

   while (!list_empty (&curr->fd_list)) {
      e = list_begin (&curr->fd_list);
      close (list_entry (e, struct fd, fd_elem)->fd_value);
    }

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
  if ( strcmp(thread_current()->name, file)==0 ) {
    file_deny_write(openfile);
  }
  
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
  struct fd * obj_fd = NULL;

  e = list_begin(&curr->fd_list);
  while ( e != list_end(&curr->fd_list) ) {
    struct fd* f = list_entry(e, struct fd, fd_elem);
    if (f->fd_value == fd) {
      obj_fd = f;
      break;
    }
    e = list_next(e);
  }

  if (!obj_fd) return ;
  file_close(obj_fd->file);
  list_remove(&obj_fd->fd_elem);
  free(obj_fd);
  return ;
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

mapid_t mmap (int fd, void *addr)
{
  struct thread * curr = thread_current ();
  struct list_elem * e;

  for (e = list_begin (&curr->fd_list); e != list_end (&curr->fd_list); e = list_next (e))
    {
      if (list_entry(e, struct fd, fd_elem)->fd_value == fd) {

        struct fd* find_fd = list_entry(e, struct fd, fd_elem);

        struct file* reopened_file = file_reopen(find_fd->file);

        if (reopened_file == NULL) 
          return -1;

        int read_bytes = file_length(reopened_file);

        int offset = 0;

        while (read_bytes > 0)
        {
          uint32_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
          uint32_t page_zero_bytes = PGSIZE - page_read_bytes;
          struct sup_page_table_entry * spte = malloc(sizeof(struct sup_page_table_entry));
          spte->user_vaddr = addr;
          spte->read_byte = page_read_bytes;
          spte->zero_byte = page_zero_bytes;
          spte->file = reopened_file;
          spte->writable = true;
          spte->flag = PAGE_MMAP;
          spte->offset = offset;
          spte->accessed = false;

          add_spte_to_table(spte);
          read_bytes -= page_read_bytes;
          addr += PGSIZE;
          offset += page_read_bytes;
        }

        int new_mapid = curr->mapid + 1;

        struct mmap_list_entry * mmap = malloc(sizeof(struct mmap_list_entry));
        mmap->mapid = new_mapid;
        mmap->user_vaddr = addr;
        mmap->file = reopened_file;
        mmap->offset = offset;

        list_push_back(&curr->mmap_list, &mmap->elem);

        curr -> mapid = new_mapid;
        return (mapid_t)curr->mapid;
      }
    }
  return -1;
}

void munmap (mapid_t mapping)
{
  // 1. FIND The appropriate mapped structure in the mmap list :) 
  struct thread * curr = thread_current ();
  struct list_elem * e;
  for (e = list_begin (&curr->mmap_list); e != list_end (&curr->mmap_list); e = list_next (e))
    {
      if (list_entry(e, struct mmap_list_entry, elem)->mapid == mapping) {

          struct mmap_list_entry * mmap = list_entry(e, struct mmap_list_entry, elem);

          int read_bytes = file_length(mmap->file);

          size_t offset = 0;

          while(read_bytes > 0)
          {
            // delete every Mapping spte from table
            void *uaddr = mmap->addr + offset;
            int bytes = (offset + PGSIZE < read_bytes ? PGSIZE : read_bytes - offset);
            remove_spte_from_table(uaddr, bytes, offset);
            offset += PGSIZE
            read_bytes -= bytes;
          }

          // Remove and free mmap structure 
          list_remove(& mmap->elem);
          file_close(mmap->file);
          free(mmap);
      }
    }
}