#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

struct frame_table_entry
{
	uint32_t* frame;
	uint32_t* upage;
	struct thread* owner;
	struct sup_page_table_entry* spte;
	struct list_elem elem;              /* List element. */
	bool locked;
};

void frame_init (void);
struct frame_table_entry* allocate_fte(enum palloc_flags flags, void* upage);
void * allocate_frame (enum palloc_flags flags, void* upage);
void * free_frame(void * frame);
void * choose_evict_frame();
void remove_all_fte(struct thread * thr);

#endif /* vm/frame.h */
