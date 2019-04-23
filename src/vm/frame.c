#include "vm/frame.h"
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
// initialize 해 줘야 하는 곳 어디? <- 여기

  list_init (&frame_table);
  lock_init (&frame_lock);

}


/* 
 * Make a new frame table entry for addr.
allocate_frame (enum palloc_flags flags) // addr는 뭘까 ? 뀨잉 ? 

 */
void *
allocate_frame (enum palloc_flags flags) // addr는 뭘까 ? 뀨잉 ? 
{

    // 1. 새로 프레임을 할당하고 
    // 2. 프레임 테이블에 집어넣는다. 
    // addr = palloc_get_page(___? );
    lock_acquire(&frame_lock);
    uint32_t *addr;
    // allocate physical memory 
    addr = palloc_get_page(flags);

    // make fte 
    struct frame_table_entry * fte = malloc(sizeof(struct frame_table_entry));
    fte -> frame = addr;
    fte -> owner = thread_current ();

    // push fte to the list
    list_push_back(&frame_table, &fte->elem);

    lock_release(&frame_lock);

    if (addr == NULL)
      return NULL;
    else
      return addr;
}

