#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <list.h>

struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	uint64_t access_time;

	bool dirty;
	bool accessed;

	struct list_elem elem;              /* List element. */
};

void page_init (void);
struct sup_page_table_entry *allocate_page (void *addr);

#endif /* vm/page.h */
