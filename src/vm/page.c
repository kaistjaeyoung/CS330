#include "vm/page.h"

#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "threads/palloc.h"

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

struct sup_page_table_entry *
allocate_page (void *addr)
{
    lock_acquire(&thread_current()->sup_lock);

    // make spte 
    struct sup_page_table_entry * spte = malloc(sizeof(struct sup_page_table_entry));
    spte->user_vaddr = addr;
    spte->accessed = true;
    spte->flag = FILE;

    // push spte to the list
    list_push_back(&thread_current()->sup_table, &spte->elem);

    lock_release(&thread_current()->sup_lock);
    return spte;
}

void page_fault_handler(void *upage, uint32_t *pagedir)
{
    // 1. table 돌면서 addr 에 해당하는 spte 있는지 찾음
    // 없으면 ㄴㄴ...
    struct sup_page_table_entry * spte = lookup_page(upage);
    if (spte == NULL)
        exit(-1);

    //2. Obtain a frame to store the page. 
    // frame_allocate로 할당하기 
    void *frame = allocate_frame(PAL_USER);
    if (frame == NULL)
        exit(-1);

    // 3. Fetch the data into the frame, by reading it from the file system or swap, zeroing it, etc.
    // 이 부분 switch 로 바꾸기
    switch(spte -> flag)
    {
      case FILE: 
        break;
      case SWAP:
        break;  
      case ALL_ZERO:
        memset (frame, 0, PGSIZE);
      default:
        exit(-1);
    }
    
    // 4. Point the page table entry for the faulting virtual address
    // to the physical page. You can use the functions in userprog/pagedir.c.
    if (!pagedir_set_page (pagedir, upage, frame, /*writable*/true)) {
        free_frame(frame);
        exit(-1);
    }

    spte->flag = FILE;
    pagedir_set_dirty (pagedir, frame, false);
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


