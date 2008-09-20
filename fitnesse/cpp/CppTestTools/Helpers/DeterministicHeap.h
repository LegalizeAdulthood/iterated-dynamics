#ifndef D_DeterministicHeap_H
#define D_DeterministicHeap_H

///////////////////////////////////////////////////////////////////////////////
//
//  DeterministicHeap.h
//
//  DeterministicHeap is responsible for managing a memory pool
//  to support deterministic new and delete operations
//
///////////////////////////////////////////////////////////////////////////////

class DeterministicHeap
  {
  public:
    explicit DeterministicHeap();
    virtual ~DeterministicHeap();

    unsigned long size();
    void* alloc(unsigned int size);
    void free(void* allocatedMemory);

  private:

    unsigned long bytesAllocated;
    char* heap;
    char* smallHeap;

    DeterministicHeap(const DeterministicHeap&);
    DeterministicHeap& operator=(const DeterministicHeap&);

  };

#endif  // D_DeterministicHeap_H
