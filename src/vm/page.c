#include "vm/page.h"

#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/malloc.h"

static struct list sup_page_table;
static struct lock sup_page_lock;

/*
 * Initialize supplementary page table
 */
void 
page_init (void)
{
// This function should locatd at Thread_init
// 1. 락 이니셜라이즈
// 2. 테이블(리스트) 이니셜라이즈 
// Where should this frame table located? <- 여기
// list 를 만들어줘야 하는 곳 어디? <- 여기
// initialize 해 줘야 하는 곳 어디? <- 여기

  list_init (&sup_page_table);
  lock_init (&sup_page_lock);
}

/*
 * Make new supplementary page table entry for addr 
 */
struct sup_page_table_entry *
allocate_page (void *addr)
{
// 1. 새로 프레임을 할당하고 
    // 2. 프레임 테이블에 집어넣는다. 
    // addr = palloc_get_page(___? );
    lock_acquire(&sup_page_lock);
    // uint32_t *addr;
    // allocate physical memory 
    // addr = palloc_get_page(flags);

    // make fte 
    struct sup_page_table_entry * spte = malloc(sizeof(struct sup_page_table_entry));
    // fte -> frame = addr;
    // fte -> owner = thread_current ();

    // push fte to the list
    list_push_back(&sup_page_table, &spte->elem);

    lock_release(&sup_page_lock);
    return spte;
}

