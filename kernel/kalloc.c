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

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];

void
kinit()
{
  for(int i=0;i<NCPU;i++)
  {
    initlock(&(kmems[i].lock), "kmem");  
  }
  
  freerange(end, (void*)PHYSTOP);
}


void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();//关中断
  int cpu_id=cpuid();
  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist;
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cpu_id=cpuid();
  
  
  acquire(&(kmems[cpu_id].lock));
  r = kmems[cpu_id].freelist;
  if(r)
  {
    //freelist有空闲
    kmems[cpu_id].freelist = r->next;
  }
  else
  {
    for(int steal_id=0;steal_id<NCPU;++steal_id)
    {
      if(steal_id==cpu_id)
      {
        continue;
      }     
      //取对象的锁
      acquire(&(kmems[steal_id].lock));
      r=kmems[steal_id].freelist;                 
      if(r)
      {
    
        kmems[steal_id].freelist = r->next; 
        release(&(kmems[steal_id].lock));
        break;
      }
      release(&(kmems[steal_id].lock));
    }         
  }
  release(&(kmems[cpu_id].lock));
  pop_off(); 

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
