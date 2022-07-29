// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"



struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

struct bucket{
  struct spinlock lock;
  struct buf bufarr[BUCKETSZ];//每一个bucket 存储的buf
};

struct bucket bhash[NBUCKETS];

int hashkey(uint key)
{
  return key%NBUCKETS;
}


void binit(void)
{
  //初始化每个锁的名称
  uint init_stamp=ticks;
  for(int i=0;i<NBUCKETS;i++)
  {
    initlock(&bhash[i].lock,"bcache");
    for(int j=0;j<BUCKETSZ;j++)
    {
      bhash[i].bufarr[j].timestamp=init_stamp;
      bhash[i].bufarr[j].bucket=i;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*  
bget(uint dev, uint blockno)
{
  int key=hashkey(blockno);
  acquire(&bhash[key].lock);
  struct buf* b;
  for(b=&bhash[key].bufarr[0]; b<&bhash[key].bufarr[0]+BUCKETSZ; b++)
  {
    if(b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;  
      b->timestamp=ticks;
      release(&bhash[key].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  uint minstamp=~0;
  struct buf* min_b=0;
  for(b=&bhash[key].bufarr[0] ; b<&bhash[key].bufarr[0]+BUCKETSZ; b++)
  {
    
    if(b->timestamp<minstamp && b->refcnt==0)
    {
      minstamp=b->timestamp;
      min_b=b;
    }
  }
  if(min_b!=0)
  {
    min_b->dev = dev;
    min_b->blockno = blockno;
    min_b->valid = 0;
    min_b->refcnt = 1;
    min_b->timestamp=ticks;
    release(&bhash[key].lock);
    acquiresleep(&min_b->lock);
    return min_b;
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bhash[b->bucket].lock); 
  b->refcnt--;
  
  release(&bhash[b->bucket].lock);
}

void
bpin(struct buf *b) {
  acquire(&bhash[b->bucket].lock);
  b->refcnt++;
  release(&bhash[b->bucket].lock);
}

void
bunpin(struct buf *b) {
  acquire(&bhash[b->bucket].lock);
  b->refcnt--;
  release(&bhash[b->bucket].lock);
}

