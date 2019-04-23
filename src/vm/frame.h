#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;
	struct list_elem elem;              /* List element. */
};

void frame_init (void);
// void * allocate_frame (enum palloc_flags flags);
void * allocate_frame (enum palloc_flags flags);

#endif /* vm/frame.h */
