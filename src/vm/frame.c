#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/malloc.h"


static struct list frame_table;
static struct lock frame_lock;

/*
 * Initialize frame table
 */
void 
frame_init (void) // Frame table init
{
// This function should locatd at Thread_init
// 1. 락 이니셜라이즈
// 2. 테이블(리스트) 이니셜라이즈 
// Where should this frame table located? <- 여기
// list 를 만들어줘야 하는 곳 어디? <- 여기
// initialize 해 줘야 하는 곳 어디? <- 여기sdfsa

  list_init (&frame_table);
  lock_init (&frame_lock);

}

struct frame_table_entry*
allocate_fte(enum palloc_flags flags, void* upage)
{
  // 1. 새로 프레임을 할당하고 
    // 2. 프레임 테이블에 집어넣는다. 
    // addr = palloc_get_page(___? );
    lock_acquire(&frame_lock);
    uint32_t *addr;
    // allocate physical memory 
    addr = palloc_get_page(flags);

    if (addr == NULL) 
    {
      struct frame_table_entry *evicted_fte = choose_evict_frame();
      ASSERT(evicted_fte->frame != NULL);
      size_t swap_index = swap_out(evicted_fte->frame);
      ASSERT( swap_index != NULL || swap_index == 0);
      struct sup_page_table_entry * spte = lookup_page(evicted_fte->upage);
      spte->flag = PAGE_SWAP;
      spte->swap_index = swap_index;
      pagedir_clear_page(evicted_fte->owner->pagedir, evicted_fte->upage);
      lock_release(&frame_lock);
      free_frame(evicted_fte->frame);
      lock_acquire(&frame_lock);
      addr = palloc_get_page (PAL_USER| flags);
      ASSERT(addr != NULL);
    }


    // make fte 
    struct frame_table_entry * fte = malloc(sizeof(struct frame_table_entry));
    fte -> frame = addr;
    fte -> owner = thread_current ();
    fte -> upage = upage;
    fte -> locked = true;

    // push fte to the list
    list_push_back(&frame_table, &fte->elem);

    lock_release(&frame_lock);

    return fte;
}


/* 
 * Make a new frame table entry for addr.
allocate_frame (enum palloc_flags flags) // addr는 뭘까 ? 뀨잉 ? 

 */
void *
allocate_frame (enum palloc_flags flags, void* upage) // addr는 뭘까 ? 뀨잉 ? (originally *addr)
{
    return allocate_fte(flags, upage)->frame;
    // 1. 새로 프레임을 할당하고 
    // 2. 프레임 테이블에 집어넣는다. 
    // addr = palloc_get_page(___? );
    // lock_acquire(&frame_lock);
    // uint32_t *addr;
    // // allocate physical memory 
    // addr = palloc_get_page(flags);

    // if (addr == NULL) 
    // {
    //   struct frame_table_entry *evicted_fte = choose_evict_frame();
    //   ASSERT(evicted_fte->frame != NULL);
    //   size_t swap_index = swap_out(evicted_fte->frame);
    //   ASSERT( swap_index != NULL || swap_index == 0);
    //   printf("allocated swap index is ? %d\n", swap_index);
    //   struct sup_page_table_entry * spte = lookup_page(evicted_fte->upage);
    //   spte->flag = PAGE_SWAP;
    //   spte->swap_index = swap_index;
    //   pagedir_clear_page(evicted_fte->owner->pagedir, evicted_fte->upage);
    //   lock_release(&frame_lock);
    //   free_frame(evicted_fte->frame);
    //   lock_acquire(&frame_lock);
    //   addr = palloc_get_page (PAL_USER| flags);
    //   ASSERT(addr != NULL);
    // }


    // // make fte 
    // struct frame_table_entry * fte = malloc(sizeof(struct frame_table_entry));
    // fte -> frame = addr;
    // fte -> owner = thread_current ();
    // fte -> upage = upage;
    // fte -> locked = true;

    // // push fte to the list
    // list_push_back(&frame_table, &fte->elem);

    // lock_release(&frame_lock);

    // return addr;
}

void *
free_frame(void * frame)
{
  lock_acquire(&frame_lock);

  // Loop list and find appropriate frame and list_remove and pointer free
  struct list_elem *e;
  for (e = list_begin(&frame_table); e != list_end(&frame_table); e = list_next(e))
    {
      struct frame_table_entry *fte = list_entry(e, struct frame_table_entry, elem);
      if (fte->frame == frame)
      {
        list_remove(e);
        free(fte);
        break;
      }
  }
  lock_release(&frame_lock);

  // free page
  palloc_free_page(frame);
}

void *
choose_evict_frame()
{
  struct list_elem *e = list_begin (&frame_table);
  ASSERT(!list_empty(&frame_table));
  while (true) {
    struct frame_table_entry *fte = list_entry(e, struct frame_table_entry, elem);
    if (e == list_end(&frame_table) ) {
      e= list_begin (&frame_table);
    } else {
      e = list_next(e);
    }

    struct thread * curr = fte->owner;

    if (fte->locked) continue;

    if (fte -> frame == NULL) continue; // I dont know why but some NULL frame inserted here..?

    if (pagedir_is_accessed(curr->pagedir, fte->upage)) {
      pagedir_set_accessed(curr->pagedir, fte->upage, false);
      continue;
    }

    ASSERT(fte -> frame != NULL);
    return fte;
  }
  PANIC("should not reached here!\n");
}
