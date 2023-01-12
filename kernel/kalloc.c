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
void freerange_noref(void *pa_start, void *pa_end);
void kfree_noref(void *pa);
void *kalloc_noref(void);
extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

#define RCAP (PGSIZE / sizeof(uint64))

uint64 ref_start, *refs;
struct spinlock ref_lock;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref_lock, "ref");
  freerange_noref(end, (void *)PHYSTOP);
  refinit();
}


void ref_acquire(void) { acquire(&ref_lock); }
void ref_release(void) { release(&ref_lock); }

// return the ref count of the page that pa belongs to
uint64 ref_get(uint64 pa)
{
  if(pa < ref_start || pa >= PHYSTOP)
    panic("ref_get: invalid pa");
  uint64 pgnum = (pa - ref_start) / PGSIZE;
  return *((uint64 *)refs[pgnum / RCAP] + (pgnum % RCAP));
}

uint64 ref_incre(uint64 pa)
{
  if(pa < ref_start || pa >= PHYSTOP)
    panic("ref_get: invalid pa");
  uint64 pgnum = (pa - ref_start) / PGSIZE;
  // printf("incre %d to %d\n", pgnum, 1 + (*((uint64 *)refs[pgnum / RCAP] + (pgnum % RCAP))));
  return ++(*((uint64 *)refs[pgnum / RCAP] + (pgnum % RCAP)));
}

uint64 ref_decre(uint64 pa)
{
  if(pa < ref_start || pa >= PHYSTOP)
    panic("ref_get: invalid pa");
  uint64 pgnum = (pa - ref_start) / PGSIZE;
  // printf("decre %d to %d\n", pgnum, (*((uint64 *)refs[pgnum / RCAP] + (pgnum % RCAP))) - 1);
  return --(*((uint64 *)refs[pgnum / RCAP] + (pgnum % RCAP)));
}

uint64 ref_check(uint64 pa, uint64 threshold)
{
  if(pa < ref_start || pa >= PHYSTOP)
    panic("ref_get: invalid pa");
  uint64 ret = 0;
  ref_acquire();
  ret = ref_get(pa);
  if (ret > threshold)
    ref_decre(pa);
  ref_release();
  return ret;
}

void refinit()
{
  refs = (uint64*)kalloc_noref();
  ref_start = PGROUNDUP((uint64)end);
  memset((void*)refs, 0, PGSIZE);
  uint64 need = (((PHYSTOP - ref_start) / PGSIZE) + (RCAP - 1)) / RCAP;
  for(int i = 0; i < need; ++i) 
  {
    refs[i] = (uint64)kalloc_noref();
    memset((void*)refs[i], 0, PGSIZE);
  }
  ref_incre((uint64)refs);
  for (int i = 0; i < need; ++i)
    ref_incre(refs[i]);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

void
freerange_noref(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree_noref(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (ref_check((uint64)pa, 0) > 1)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
kfree_noref(void *pa)
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
  {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
  {
    if (ref_get((uint64)r) != 0)
    {
      printf("kalloc free mem %p ref count = %d != 0", (uint64)r, ref_get((uint64)r));
      panic("kalloc");
    }
    ref_incre((uint64)r);
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc_noref(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
  {
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
