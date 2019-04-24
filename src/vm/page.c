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
  void*kpage,
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
    lock_acquire(&thread_current()->sup_lock);

    // make spte 
    struct sup_page_table_entry * spte = malloc(sizeof(struct sup_page_table_entry));
    spte->user_vaddr = upage;
    spte->accessed = true;
    spte->flag = flag;
    spte->read_byte = read_byte;
    spte->zero_byte = zero_byte;
    spte->file = file;
    spte->writable = writable;

    // push spte to the list
    list_push_back(&thread_current()->sup_table, &spte->elem);

    lock_release(&thread_current()->sup_lock);
    return spte;
}

struct sup_page_table_entry *
add_spte_to_table(struct sup_page_table_entry *spte)
{
  lock_acquire(&thread_current()->sup_lock);
  list_push_back(&thread_current()->sup_table, &spte->elem);
  lock_release(&thread_current()->sup_lock);
}

bool page_fault_handler(void *upage, uint32_t *pagedir)
{
    // 1. table 돌면서 addr 에 해당하는 spte 있는지 찾음
    // 없으면 ㄴㄴ...
    struct sup_page_table_entry * spte = lookup_page(upage);
    if (spte == NULL) {
        printf("in first\n");
        exit(-1);
    }

    //2. Obtain a frame to store the page. 
    // frame_allocate로 할당하기 
    void *frame = allocate_frame(PAL_USER);
    if (frame == NULL) {
        printf("in second\n");
        exit(-1);
    }

    // 3. Fetch the data into the frame, by reading it from the file system or swap, zeroing it, etc.
    // 이 부분 switch 로 바꾸기
    switch(spte -> flag)
    {
      case PAGE_FILE: 
        break;
      case PAGE_SWAP:
        break;  
      case PAGE_ALL_ZERO:
        memset (frame, 0, PGSIZE);
        break;
      case PAGE_MMAP:
        if (!handle_page_fault_mmap (
          spte->user_vaddr,
          frame,
          spte->read_byte,
          spte->zero_byte,
          spte->file,
          spte->writable,
          spte->offset
        ))
          printf("not working!!!!!!!\n");
        // memset (frame, 0, PGSIZE);

        break;
      default:
        exit(-1);
    }
    
    // 4. Point the page table entry for the faulting virtual address
    // to the physical page. You can use the functions in userprog/pagedir.c.
    // if (!pagedir_set_page (pagedir, upage, frame, /*writable*/true)) {
    //     free_frame(frame);
    //     printf("in fourth\n");
    //     exit(-1);
    // }

    spte->flag = PAGE_FILE;
    // pagedir_set_dirty (pagedir, frame, false);
    return true;
}

bool
handle_page_fault_mmap(
  void *upage,
  void*kpage,
  size_t read_byte,
  size_t zero_byte,
  struct file* file,
  bool writable,
  int offset
  )
{
  printf("come to this????\n");
  printf("read_Byte is? %d\n", read_byte);
  int i = file_read_at (file, kpage, read_byte, offset);
  printf("file read bye is? %d\n", i);
  printf("received offset is? %d\n", offset);

  if (file_read_at (file, kpage, read_byte, offset) != (int) read_byte)
  {
    printf("wrong here/????!\n");
    free_frame (kpage);
    return false; 
  }
  memset (kpage + read_byte, 0, zero_byte);

  /* Add the page to the process's address space. */
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
    address, then map our page there. */
  // JH COMMENT : 여기서 page set 해주는데 supt entry 도 같이 세팅해 줘야 함~~
  printf("wrong here?1\n");
  bool success = pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable);
  printf("wrong here?2\n");
  if (!success) 
    {
      free_frame (kpage);
      return false; 
    }
  return success;
}

struct sup_page_table_entry *
lookup_page(void *addr)
{
  struct list_elem *e;
  for (e = list_begin(&thread_current()->sup_table); e != list_end(&thread_current()->sup_table); e = list_next(e))
    {
      struct sup_page_table_entry *spte = list_entry(e, struct sup_page_table_entry, elem);
      if (spte->user_vaddr == addr)
      {
        return spte;
      }
  }
  return NULL;
}