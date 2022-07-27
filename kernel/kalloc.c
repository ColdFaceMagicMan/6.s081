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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct refcount{
  struct spinlock lock;
  int num;
};
struct refcount ref_count[(PHYSTOP - KERNBASE)/PGSIZE];

void add_ref_count(void* pa)
{
  uint64 i=((uint64)pa-KERNBASE)/PGSIZE;
  acquire(&ref_count[i].lock);
  ++ref_count[i].num;
  release(&ref_count[i].lock);
}

void dec_ref_count(void* pa)
{
  uint64 i=((uint64)pa-KERNBASE)/PGSIZE;
  acquire(&ref_count[i].lock);
  --ref_count[i].num;
  release(&ref_count[i].lock);
}

int get_ref_count(void* pa)
{
  uint64 i=((uint64)pa-KERNBASE)/PGSIZE;
  acquire(&ref_count[i].lock);
  int num=ref_count[((uint64)pa-KERNBASE)/PGSIZE].num;
  release(&ref_count[i].lock);
  return num;
}

void
kinit()
{
  for(int i=0;i<(PHYSTOP - KERNBASE)/PGSIZE;i++)
  {
    initlock(&(ref_count[i].lock),"page_ref");
  }
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    ref_count[((uint64)p-KERNBASE)/PGSIZE].num=1;
    kfree(p);    
  }

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


  uint64 i=((uint64)pa-KERNBASE)/PGSIZE;
  acquire(&ref_count[i].lock);
  if(ref_count[i].num==0)//说明发生重复释放
  {
    release(&ref_count[i].lock);  
    panic("kfree: too much times");
  }
  ref_count[i].num--;
  if(ref_count[i].num!=0)
  {
    release(&ref_count[i].lock);
    return;
  }
  else//在引用计数为0时才真正释放
  {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
    release(&ref_count[i].lock);    
  }

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
  if(r){
    uint64 i=((uint64)r-KERNBASE)/PGSIZE;
    acquire(&ref_count[i].lock);
    ref_count[i].num=1;
    release(&ref_count[i].lock);
    kmem.freelist = r->next;   //空闲指针指向下一个
  }

  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}





