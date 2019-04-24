#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <list.h>

/*
The supplemental page table is used for at least two purposes.
Most importantly, on a page fault, the kernel looks up the virtual page that faulted in the
supplemental page table to find out what data should be there. Second, the kernel consults
the supplemental page table when a process terminates, to decide what resources to free.
*/

/* How to allocate pages. */
enum spte_flags
  {
    FILE = 001,           /* Panic on failure. */
    SWAP = 002,             /* Zero page contents. */
    ALL_ZERO = 003              /* User page. */
  };


struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	uint64_t access_time;

	bool dirty;
	bool accessed;

	enum spte_flags flag;

	// page_table_entry status 만들어야 함. 

	struct list_elem elem;              /* List element. */
};

void page_init (void);
struct sup_page_table_entry *allocate_page (void *addr);
void page_fault_handler(void *upage, uint32_t *pagedir);
struct sup_page_table_entry *lookup_page(void *addr);

#endif /* vm/page.h */
