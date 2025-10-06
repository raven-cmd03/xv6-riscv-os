// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);
void superfreerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// Superpage allocation structures
struct superpage_run {
  struct superpage_run *next;
};

struct {
  struct spinlock lock;
  struct superpage_run *freelist;
} superkmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&superkmem.lock, "superkmem");
  freerange(end, (void*)PHYSTOP);
  // Disable superpage initialization during boot for now
  // superfreerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Initialize superpage free list
void
superfreerange(void *pa_start, void *pa_end)
{
  char *p;
  int count = 0;
  p = (char*)SUPERPGROUNDUP((uint64)pa_start);
  
  for(; p + SUPERPGSIZE <= (char*)pa_end && count < 10; p += SUPERPGSIZE) {
    // Only add superpages if they don't conflict with regular page allocations
    // Skip the first few superpages to avoid conflicts
    if((uint64)p > (uint64)end + 10 * SUPERPGSIZE) {
      superfree(p);
      count++;
    }
  }
  printf("superfreerange: initialized %d superpages\n", count);
}

// Free a superpage
void
superfree(void *pa)
{
  struct superpage_run *r;

  if(((uint64)pa % SUPERPGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("superfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, SUPERPGSIZE);

  r = (struct superpage_run*)pa;

  acquire(&superkmem.lock);
  r->next = superkmem.freelist;
  superkmem.freelist = r;
  release(&superkmem.lock);
}

// Allocate one superpage (2MB) of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
superalloc(void)
{
  struct superpage_run *r;

  acquire(&superkmem.lock);
  r = superkmem.freelist;
  
  // If no superpages available, try to initialize some
  if(r == 0) {
    release(&superkmem.lock);
    // Initialize superpages lazily when first needed
    superfreerange((void*)end, (void*)PHYSTOP);
    acquire(&superkmem.lock);
    r = superkmem.freelist;
  }
  
  if(r)
    superkmem.freelist = r->next;
  release(&superkmem.lock);

  if(r)
    memset((char*)r, 5, SUPERPGSIZE); // fill with junk
  return (void*)r;
}
