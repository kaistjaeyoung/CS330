#include "vm/swap.h"
#include "devices/disk.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <bitmap.h>

/* The swap device */
static struct disk *swap_device;

/* Tracks in-use and free swap slots */
static struct bitmap *swap_table;

/* Protects swap_table */
static struct lock swap_lock;


static size_t swap_size;

static const size_t SECTOR_NUMBER = PGSIZE / DISK_SECTOR_SIZE;

/* 
 * Initialize swap_device, swap_table, and swap_lock.
 */
void 
swap_init (void)
{
  swap_device = disk_get (1, 1);
  if (swap_device == NULL) {
      PANIC("Should not reached here -- swap.c swap_init()");
  } 
  swap_size = disk_size(swap_device) / SECTOR_NUMBER;
  swap_table = bitmap_create(swap_size);
  ASSERT(swap_table != NULL);
  bitmap_set_all(swap_table, true);
  lock_init(&swap_lock);
}

/*
 * Reclaim a frame from swap device.
 * 1. Check that the page has been already evicted. 
 * 2. You will want to evict an already existing frame
 * to make space to read from the disk to cache. 
 * 3. Re-link the new frame with the corresponding supplementary
 * page table entry. 
 * 4. Do NOT create a new supplementray page table entry. Use the 
 * already existing one. 
 * 5. Use helper function read_from_disk in order to read the contents
 * of the disk into the frame. 
 */ 
bool 
swap_in (void *addr, int index)
{
  if (!swap_table || !swap_device) {
        PANIC("should be initialized before swap");
  }

  ASSERT(bitmap_test(swap_table, index) == false);
  bitmap_set(swap_table, index, true);
  read_from_disk(addr, index);
}

/* 
 * Evict a frame to swap device. 
 * 1. Choose the frame you want to evict. 
 * (Ex. Least Recently Used policy -> Compare the timestamps when each 
 * frame is last accessed)
 * 2. Evict the frame. Unlink the frame from the supplementray page table entry
 * Remove the frame from the frame table after freeing the frame with
 * pagedir_clear_page. 
 * 3. Do NOT delete the supplementary page table entry. The process
 * should have the illusion that they still have the page allocated to
 * them. 
 * 4. Find a free block to write you data. Use swap table to get track
 * of in-use and free swap slots.
 */
size_t
swap_out (void *frame)
{
    if (!swap_table || !swap_device) {
        PANIC("should be initialized before swap");
    }
    ASSERT (frame != NULL);
    lock_acquire(&swap_lock);
    size_t swap_index = bitmap_scan_and_flip (swap_table, 0, 1, true);
    write_to_disk(frame, swap_index);
    // bitmap_set(swap_table, swap_index, false);
    ASSERT(swap_index != BITMAP_ERROR);
    lock_release(&swap_lock);
    return swap_index;
}

/* 
 * Read data from swap device to frame. 
 * Look at device/disk.c
 */
void read_from_disk (uint8_t *frame, int index)
{
  lock_acquire(&swap_lock);
  int i;
  for (i = 0; i < SECTOR_NUMBER; ++ i) {
      disk_read (
          swap_device,
          index * SECTOR_NUMBER + i,
          frame + (DISK_SECTOR_SIZE * i)
          );
  };
  lock_release(&swap_lock);
  return;
}

/* Write data to swap device from frame */
void write_to_disk (uint8_t *frame, int index)
{
    int i;
    for (i = 0; i < SECTOR_NUMBER; ++ i) {
        disk_write (
            swap_device,
            index * SECTOR_NUMBER + i,
            frame + (DISK_SECTOR_SIZE * i)
            );
    };
    return;
}

