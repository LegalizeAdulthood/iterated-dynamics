#include "UnitTestHarness/TestHarness.h"
#include "DeterministicHeap.h"
#include <string.h>

EXPORT_TEST_GROUP(DeterministicHeap);

namespace
  {
  DeterministicHeap* heap;

  void SetUp()
  {
    heap = new DeterministicHeap();
  }
  void TearDown()
  {
    delete heap;
  }
}


// DeterministicHeap is a work in progress, and not ready to use

TEST(DeterministicHeap, Create)
{
  LONGS_EQUAL(0, heap->size());
}

TEST(DeterministicHeap, heapGrows)
{
  void* mem = heap->alloc(100);
  int size = heap->size();
  if (size < 100)
    FAIL("Heap size should be >= 100");

  heap->free(mem);
  LONGS_EQUAL(size, heap->size());
}

TEST(DeterministicHeap, allocGiveMemory)
{
  void* mem = heap->alloc(100);
  CHECK(0 != mem);
}

TEST(DeterministicHeap, getBackTheSameBlock)
{
  char* mem1 = static_cast<char*>(heap->alloc(100));
  heap->free(mem1);
  char* mem2 = static_cast<char*>(heap->alloc(100));
  CHECK_EQUAL(mem1, mem2);
  heap->free(mem2);
}

TEST(DeterministicHeap, differentBlockSizes)
{
  char* bigMem1 = static_cast<char*>(heap->alloc(100));
  char* smallMem1 = static_cast<char*>(heap->alloc(10));
  heap->free(bigMem1);
  heap->free(smallMem1);
  char* bigMem2 = static_cast<char*>(heap->alloc(100));
  char* smallMem2 = static_cast<char*>(heap->alloc(10));
  CHECK_EQUAL(bigMem1, bigMem2);
  CHECK_EQUAL(smallMem1, smallMem2);
  CHECK(smallMem1 != bigMem1);
}

