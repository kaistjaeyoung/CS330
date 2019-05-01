#include "vm/page.h"

#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "threads/palloc.h"
#include "filesys/file.h"
#include "userprog/syscall.h"


// static struct list sup_page_table;
// static struct lock sup_page_lock;

/*
 * Initialize supplementary page table

 */


void 
page_init (void)
{
// This function should locatd at Thread_init
// 1. 락 이니셜라이즈
// 2. 테이블(리스트) 이니셜라이즈 
// Where should this sup page table located? <- per process~ here?
// list 를 만들어줘야 하는 곳 어디? <- 여기
// initialize 해 줘야 하는 곳 어디? <- 여기

  list_init (&thread_current()->sup_table);
  lock_init (&thread_current()->sup_lock);
}

/*
 * Make new supplementary page table entry for addr 
 * Supplementary page table should be allocated when the page_dir created
 */

bool
allocate_page (
  void *upage,
  void *kpage,
  enum spte_flags flag,
  size_t read_byte,
  size_t zero_byte,
  struct file* file,
  bool writable
  )
{
     if (file_read (file, kpage, read_byte) != (int) read_byte)
        {
          free_frame (kpage);
          return false; 
        }
      memset (kpage + read_byte, 0, zero_byte);

      /* Add the page to the process's address space. */
      struct thread *t = thread_current ();

      /* Verify that there's not already a page at that virtual
        address, then map our page there. */
      // JH COMMENT : 여기서 page set 해주는데 supt entry 도 같이 세팅해 줘야 함~~
      bool success = pagedir_get_page (t->pagedir, upage) == NULL
              && pagedir_set_page (t->pagedir, upage, kpage, writable);
      if (!success) 
        {
          free_frame (kpage);
          return false; 
        }

    // make spte 
    struct sup_page_table_entry * spte = malloc(sizeof(struct sup_page_table_entry));
    spte->user_vaddr = upage;
    spte->accessed = true;
    spte->flag = flag;
    spte->read_byte = read_byte;
    spte->zero_byte = zero_byte;
    spte->file = file;
    spte->writable = writable;
    spte->dirty = false;
    spte->frame = kpage;

    // push spte to the list
    lock_acquire(&thread_current()->sup_lock);
    list_push_back(&thread_current()->sup_table, &spte->elem);
    lock_release(&thread_current()->sup_lock);
    return true;
}

bool
add_spte_to_table(struct sup_page_table_entry *spte)
{
  if (lookup_page(spte->user_vaddr)) {
    // printf("comes to lookup page false\n");
    return false;

  }
  // printf("comes to lookup page before lock\n");
  lock_acquire(&thread_current()->sup_lock);
  list_push_back(&thread_current()->sup_table, &spte->elem);
  lock_release(&thread_current()->sup_lock);
  // printf("comes to lookup page after lock\n");
  return true;
}

struct sup_page_table_entry *
find_spte(void * uaddr)
{
  struct thread * curr = thread_current ();
  struct list_elem * e;
  // lock_acquire(&curr->sup_lock);
  for (e = list_begin (&curr->sup_table); e != list_end (&curr->sup_table); e = list_next (e))
  {
    struct sup_page_table_entry * spte = list_entry(e, struct sup_page_table_entry, elem);
      if (spte->user_vaddr == uaddr)
        // lock_release(&curr->sup_lock);
        return spte;
  }
  // lock_release(&curr->sup_lock);
  return NULL;
}

bool remove_spte_from_table(void *upage, size_t byte, size_t offset)
{
    struct thread* curr = thread_current();

    struct sup_page_table_entry * spte = find_spte(upage);

    if (spte == NULL) {
      return false;
    }

    switch(spte -> flag)
    {
      case PAGE_FILE: 
        break;
      case PAGE_SWAP:
        break;  
      case PAGE_ALL_ZERO:
        break;
      case PAGE_MMAP:
        break;
      case PAGE_LOADED:
        if (pagedir_is_dirty(curr->pagedir, upage) || pagedir_is_dirty(curr->pagedir, spte->frame))
        {
          file_write_at(spte->file, spte->user_vaddr, spte->read_byte, spte->offset);
        }
        free_frame (spte->frame);
        pagedir_clear_page (curr->pagedir, spte->user_vaddr);
        break;
      default:
        return false;
    }

  list_remove(&spte->elem);
  return true;
}

bool page_fault_handler(void *upage, uint32_t *pagedir)
{
    struct thread* curr = thread_current ();

    struct sup_page_table_entry * spte = lookup_page(upage);

    void * frame;

    if (spte == NULL) {
        exit(-1);
    }

    bool loaded = false;

    switch(spte -> flag)
    {
      case PAGE_FILE:
        if (!handle_page_fault_mmap (spte)) {
          exit(-1);
        }
        loaded = true;
        break;
      case PAGE_SWAP:
        printf("come here page_swap : 0x%x\n", upage);
        // loaded = true;
        break;  
      case PAGE_ALL_ZERO:
        frame = allocate_frame(PAL_USER, upage);
        if (frame == NULL) {
            exit(-1);
        }
        memset (frame, 0, PGSIZE);
        break;
      case PAGE_MMAP:
        if (!handle_page_fault_mmap (spte)) {
          printf("come here page_file? why?? x%0x\n", upage);
          exit(-1);
        }
        loaded = true;
        break;
      case PAGE_LOADED:
        loaded = true;
        break;
      default:
        exit(-1);
    }

    return loaded;
}

bool
handle_page_fault_mmap(struct sup_page_table_entry * spte)
{
  void *frame = allocate_frame(PAL_USER, spte->user_vaddr);
  if (frame == NULL)
      return false;

  lock_acquire(&syscall_lock);
  if (file_read_at (spte->file, frame, spte->read_byte, spte->offset) != (int) spte->read_byte)
  {
    lock_release(&syscall_lock);
    free_frame (frame);
    return false; 
  }
  lock_release(&syscall_lock);

  memset (frame + spte->read_byte, 0, spte->zero_byte);

  struct thread *t = thread_current ();

  bool success = pagedir_get_page (t->pagedir, spte->user_vaddr) == NULL
          && pagedir_set_page (t->pagedir, spte->user_vaddr, frame, spte->writable);
  if (!success) 
    {
      free_frame (frame);
      return false; 
    }

  lock_acquire(&t->sup_lock);
  spte->frame = frame;
  spte->flag = PAGE_LOADED;
  lock_release(&t->sup_lock);

  pagedir_set_dirty (t->pagedir, frame, false);
  return success;
}

struct sup_page_table_entry *
lookup_page(void *addr)
{
  struct list_elem *e;
  struct thread* curr = thread_current ();
  lock_acquire(&curr->sup_lock);
  for (e = list_begin(&curr->sup_table); e != list_end(&curr->sup_table); e = list_next(e))
    {
      struct sup_page_table_entry *spte = list_entry(e, struct sup_page_table_entry, elem);
      if (spte->user_vaddr == addr)
      {
        lock_release(&curr->sup_lock);
        return spte;
      }
    }
  lock_release(&curr->sup_lock);
  return NULL;
}

/* jjy implement */
/**
 * Install a page (specified by the starting address `upage`)
 * on the supplemental page table. The page is of type ALL_ZERO,
 * indicates that all the bytes is (lazily) zero.
 */
void
spt_install_new_zeropage (void *upage)
{

  struct sup_page_table_entry *spte;
  spte = (struct sup_page_table_entry *) malloc(sizeof(struct sup_page_table_entry));

  spte->user_vaddr = upage;
  spte->flag = PAGE_ALL_ZERO;

  if (! add_spte_to_table(spte) ) {
    printf("comes to in [if (! add_spte_to_table(spte) ) ]\n");
    PANIC("Duplicated SUPT entry for zeropage");
    return ;
  }
  printf("success to install_new_zeropage\n");
  return;
}
/* jjy implement */
