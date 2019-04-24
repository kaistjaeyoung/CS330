#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <list.h>

/*
The supplemental page table is used for at least two purposes.
Most importantly, on a page fault, the kernel looks up the virtual page that faulted in the
supplemental page table to find out what data should be there. Second, the kernel consults
the supplemental page table when a process terminates, to decide what resources to free.
*/

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
void page_fault_handler(void *upage, uint32_t *pagedir);
struct sup_page_table_entry *lookup_page(void *addr);

#endif /* vm/page.h */
