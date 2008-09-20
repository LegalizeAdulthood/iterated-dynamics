#include "DeterministicHeap.h"
#include <memory>

DeterministicHeap::DeterministicHeap()
    : bytesAllocated(0)
    , heap(0)
    , smallHeap(0)
{}

DeterministicHeap::~DeterministicHeap()
{
  delete [] heap;
  delete [] smallHeap;
}

unsigned long DeterministicHeap::size()
{
  return bytesAllocated;
}

void* DeterministicHeap::alloc(unsigned int size)
{
  //this is a work in progress
  //this code needs to evolve to handle
  //more than two block sizes and
  //more than one block of each size
  if (size <= 50)
    {
      if (smallHeap == 0)
        {
          smallHeap = new char[50 + 4];
          *(int*)smallHeap = 50;
        }
      return smallHeap + 4;
    }

  bytesAllocated += 104;
  if (heap == 0)
    {
      heap = new char[100 + 4];
      *(int*)heap = 100;
    }
  return heap + 4;
}

void DeterministicHeap::free(void* allocatedMemory)
{
  char* theap = static_cast<char*>(allocatedMemory) - 4;
  if (*(int*)theap == 50)
    smallHeap = theap;
  else
    heap = theap;



}
