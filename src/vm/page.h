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
    PAGE_FILE = 001,          
    PAGE_SWAP = 002,           
    PAGE_ALL_ZERO = 003,
	PAGE_MMAP = 004,           
  };


struct sup_page_table_entry 
{
	uint32_t* user_vaddr;
	uint64_t access_time;
	size_t read_byte;
	size_t zero_byte;
	struct file* file;
	bool writable;

	bool dirty;
	bool accessed;

	enum spte_flags flag;

	struct list_elem elem;              /* List element. */
};

void page_init (void);
bool allocate_page (void *upage, void*kpage, enum spte_flags flag, size_t read_byte, size_t zero_byte, struct file* file, bool writable);
bool page_fault_handler(void *upage, uint32_t *pagedir);
struct sup_page_table_entry *lookup_page(void *addr);

#endif /* vm/page.h */
