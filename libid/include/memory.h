#pragma once

enum stored_at_values
{
    NOWHERE,
    MEMORY,
    DISK
};

// memory -- C file prototypes
// TODO: Get rid of this and use regular memory routines;
// see about creating standard disk memory routines for disk video
void DisplayHandle(U16 handle);
int MemoryType(U16 handle);
void InitMemory();
void ExitCheck();
U16 MemoryAlloc(U16 size, long count, int stored_at);
void MemoryRelease(U16 handle);
bool CopyFromMemoryToHandle(BYTE const *buffer, U16 size, long count, long offset, U16 handle);
bool CopyFromHandleToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
bool SetMemory(int value, U16 size, long count, long offset, U16 handle);
