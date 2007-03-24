#include <string.h>
#include <limits.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <ctype.h>
#include <errno.h>

#include "port.h"
#include "prototyp.h"
#include "drivers.h"

/* Memory allocation routines. */

/* For far memory: */
#define FAR_RESERVE   8192L    /* amount of far mem we will leave avail. */
/* For disk memory: */
#define DISKWRITELEN 2048L /* max # bytes transferred to/from disk mem at once */

BYTE *charbuf = NULL;
//int numEXThandles;
//long ext_xfer_size;
//U16 start_avail_extra = 0;

#define MAXHANDLES 256   /* arbitrary #, suitably big */
char memfile[] = "handle.$$$";
int numTOTALhandles;

char memstr[3][9] = {{"nowhere"}, {"memory"}, {"disk"}};

struct nowhere {
   enum stored_at_values stored_at; /* first 2 entries must be the same */
   long size;                       /* for each of these data structures */
   };

struct linearmem {
   enum stored_at_values stored_at;
   long size;
   BYTE *memory;
   };

struct disk {
   enum stored_at_values stored_at;
   long size;
   FILE *file;
   };

union mem {
   struct nowhere Nowhere;
   struct linearmem Linearmem;
   struct disk Disk;
   };

union mem handletable[MAXHANDLES];

/* Routines in this module */
static int _fastcall  CheckDiskSpace(long howmuch);
static int check_for_mem(int stored_at, long howmuch);
static U16 next_handle(void);
static int CheckBounds (long start, long length, U16 handle);
static void WhichDiskError(int);
static void DisplayError(int stored_at, long howmuch);

/* Routines in this module, visible to outside routines */

void DisplayMemory (void);
void DisplayHandle (U16 handle);
int MemoryType (U16 handle);
void InitMemory (void);
void ExitCheck (void);
U16 MemoryAlloc(U16 size, long count, int stored_at);
void MemoryRelease(U16 handle);
int MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
int MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle);
int SetMemory(int value, U16 size, long count, long offset, U16 handle);

/* Memory handling support routines */

static int _fastcall CheckDiskSpace(long howmuch)
{
	/* TODO */
	return TRUE;
}

static void WhichDiskError(int I_O)
{
	/* Set I_O == 1 after a file create, I_O == 2 after a file set value */
	/* Set I_O == 3 after a file write, I_O == 4 after a file read */
	char buf[MSGLEN];
	char *pats[4] =
	{
		"Create file error %d:  %s",
		"Set file error %d:  %s",
		"Write file error %d:  %s",
		"Read file error %d:  %s"
	};
	sprintf(buf, pats[(1 <= I_O && I_O <= 4) ? (I_O-1) : 0], errno, strerror(errno));
	if (debugflag == 10000)
		if (stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER, (char *)buf) == -1)
			goodbye(); /* bailout if ESC */
}

int MemoryType(U16 handle)
{
   return handletable[handle].Nowhere.stored_at;
}

static void DisplayError(int stored_at, long howmuch)
{
/* This routine is used to display an error message when the requested */
/* memory type cannot be allocated due to insufficient memory, AND there */
/* is also insufficient disk space to use as memory. */

   char buf[MSGLEN*2];
   sprintf(buf, "Allocating %ld Bytes of %s memory failed.\n"
	   "Alternate disk space is also insufficient. Goodbye",
	   howmuch, memstr[stored_at]);
   stopmsg(0, buf);
}

static int check_for_mem(int stored_at, long howmuch)
{
	/* This function returns an adjusted stored_at value. */
	/* This is where the memory requested can be allocated. */
	long maxmem = (1 << 20);
	BYTE *temp;
	int use_this_type = NOWHERE;

	if (debugflag == 420)
	{
		stored_at = DISK;
	}
	if (debugflag == 422)
	{
		stored_at = MEMORY;
	}

	switch (stored_at)
	{
	case MEMORY: /* check_for_mem */
		if (maxmem > howmuch)
		{
			temp = (BYTE *) malloc(howmuch);
			if (temp != NULL) /* minimum free space + requested amount */
			{
				free(temp);
				use_this_type = MEMORY;
				break;
			}
		}

	case DISK: /* check_for_mem */
	default: /* just in case a nonsense number gets used */
		if (CheckDiskSpace(howmuch))
		{
			use_this_type = DISK;
			break;
		}
		/* failed, fall through, no memory available */

	case NOWHERE: /* check_for_mem */
		use_this_type = NOWHERE;
		break;
   } /* end of switch */

   return use_this_type;
}

static U16 next_handle()
{
   U16 counter = 1; /* don't use handle 0 */

   while (handletable[counter].Nowhere.stored_at != NOWHERE &&
         counter < MAXHANDLES)
      counter++;
   return counter;
}

static int CheckBounds (long start, long length, U16 handle)
{
   if (handletable[handle].Nowhere.size - start - length < 0)
      {
       stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Memory reference out of bounds.");
       DisplayHandle(handle);
       return 1;
      }
   if (length > (long)USHRT_MAX)
      {
       stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Tried to move > 65,535 bytes.");
       DisplayHandle(handle);
       return 1;
      }
   if (handletable[handle].Nowhere.stored_at == DISK &&
         (stackavail() <= DISKWRITELEN))
      {
       stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Stack space insufficient for disk memory.");
       DisplayHandle(handle);
       return 1;
      }
   if (length <= 0)
      {
       stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Zero or negative length.");
       DisplayHandle(handle);
       return 1;
      }
   if (start < 0)
      {
       stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Negative offset.");
       DisplayHandle(handle);
       return 1;
      }
   return 0;
}

void DisplayMemory (void)
{
   char buf[MSGLEN];
   extern unsigned long get_disk_space(void);

   sprintf(buf, "disk=%lu", get_disk_space());
   stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, buf);
}

void DisplayHandle (U16 handle)
{
   char buf[MSGLEN];

   sprintf(buf, "Handle %u, type %s, size %li", handle, memstr[handletable[handle].Nowhere.stored_at],
           handletable[handle].Nowhere.size);
   if (stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER, (char *)buf) == -1)
     goodbye(); /* bailout if ESC, it's messy, but should work */
}

void InitMemory (void)
{
	int counter;

	numTOTALhandles = 0;
	for (counter = 0; counter < MAXHANDLES; counter++)
	{
		handletable[counter].Nowhere.stored_at = NOWHERE;
		handletable[counter].Nowhere.size = 0;
	}
}

void ExitCheck (void)
{
   U16 i;
   if (numTOTALhandles != 0) {
      stopmsg(0, "Error - not all memory released, I'll get it.");
      for (i = 1; i < MAXHANDLES; i++)
         if (handletable[i].Nowhere.stored_at != NOWHERE) {
            char buf[MSGLEN];
            sprintf(buf, "Memory type %s still allocated.  Handle = %i.",
				memstr[handletable[i].Nowhere.stored_at], i);
            stopmsg(0, (char *)buf);
            MemoryRelease(i);
         }
   }
}

/* * * * * */
/* Memory handling routines */

U16 MemoryAlloc(U16 size, long count, int stored_at)
{
/* Returns handle number if successful, 0 or NULL if failure */
   U16 handle = 0;
   int success, use_this_type;
   long toallocate;

   success = FALSE;
   toallocate = count * size;
   if (toallocate <= 0)     /* we failed, can't allocate > 2,147,483,647 */
      return (U16)success; /* or it wraps around to negative */

/* check structure for requested memory type (add em up) to see if
   sufficient amount is available to grant request */

   use_this_type = check_for_mem(stored_at, toallocate);
   if (use_this_type == NOWHERE) {
      DisplayError(stored_at, toallocate);
      goodbye();
   }

/* get next available handle */

   handle = next_handle();

   if (handle >= MAXHANDLES || handle <= 0) {
      DisplayHandle(handle);
      return (U16)success;
   /* Oops, do something about this! ????? */
   }

/* attempt to allocate requested memory type */
   switch (use_this_type)
   {
   case NOWHERE: /* MemoryAlloc */
      use_this_type = NOWHERE; /* in case nonsense value is passed */
#if defined(_WIN32)
	  _ASSERTE(use_this_type != NOWHERE);
#endif
      break;

   case MEMORY: /* MemoryAlloc */
/* Availability of memory checked in check_for_mem() */
      handletable[handle].Linearmem.memory = (BYTE *)malloc(toallocate);
      handletable[handle].Linearmem.size = toallocate;
      handletable[handle].Linearmem.stored_at = MEMORY;
      numTOTALhandles++;
      success = TRUE;
      break;

   default:
   case DISK: /* MemoryAlloc */
      memfile[9] = (char)(handle % 10 + (int)'0');
      memfile[8] = (char)((handle % 100) / 10 + (int)'0');
      memfile[7] = (char)((handle % 1000) / 100 + (int)'0');
      if (disktarga)
         handletable[handle].Disk.file = dir_fopen(workdir, light_name, "a+b");
      else
         handletable[handle].Disk.file = dir_fopen(tempdir, memfile, "w+b");
      rewind(handletable[handle].Disk.file);
      if (fseek(handletable[handle].Disk.file, toallocate, SEEK_SET) != 0)
         handletable[handle].Disk.file = NULL;
      if (handletable[handle].Disk.file == NULL) {
         handletable[handle].Disk.stored_at = NOWHERE;
         use_this_type = NOWHERE;
         WhichDiskError(1);
         DisplayMemory();
         driver_buzzer(BUZZER_ERROR);
         break;
      }
      numTOTALhandles++;
      success = TRUE;
      fclose(handletable[handle].Disk.file); /* so clusters aren't lost if we crash while running */
      if (disktarga)
         handletable[handle].Disk.file = dir_fopen(workdir, light_name, "r+b");
      else
         handletable[handle].Disk.file = dir_fopen(tempdir, memfile, "r+b"); /* reopen */
      rewind(handletable[handle].Disk.file);
      handletable[handle].Disk.size = toallocate;
      handletable[handle].Disk.stored_at = DISK;
      use_this_type = DISK;
      break;
   } /* end of switch */

   if (stored_at != use_this_type && debugflag == 10000) {
      char buf[MSGLEN];
      sprintf(buf, "Asked for %s, allocated %lu bytes of %s, handle = %u.",
		  memstr[stored_at], toallocate, memstr[use_this_type], handle);
      stopmsg(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, (char *)buf);
      DisplayMemory();
   }

   if (success)
      return handle;
   else      /* return 0 if failure */
      return 0;
}

void MemoryRelease(U16 handle)
{
   switch (handletable[handle].Nowhere.stored_at)
   {
   case NOWHERE: /* MemoryRelease */
      break;

   case MEMORY: /* MemoryRelease */
      free(handletable[handle].Linearmem.memory);
      handletable[handle].Linearmem.memory = NULL;
      handletable[handle].Linearmem.size = 0;
      handletable[handle].Linearmem.stored_at = NOWHERE;
      numTOTALhandles--;
      break;

   case DISK: /* MemoryRelease */
      memfile[9] = (char)(handle % 10 + (int)'0');
      memfile[8] = (char)((handle % 100) / 10 + (int)'0');
      memfile[7] = (char)((handle % 1000) / 100 + (int)'0');
      fclose(handletable[handle].Disk.file);
      dir_remove(tempdir, memfile);
      handletable[handle].Disk.file = NULL;
      handletable[handle].Disk.size = 0;
      handletable[handle].Disk.stored_at = NOWHERE;
      numTOTALhandles--;
      break;
   } /* end of switch */
}

int MoveToMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle)
{ /* buffer is a pointer to local memory */
/* Always start moving from the beginning of buffer */
/* offset is the number of units from the start of the allocated "Memory" */
/* to start moving the contents of buffer to */
/* size is the size of the unit, count is the number of units to move */
/* Returns TRUE if successful, FALSE if failure */
   BYTE diskbuf[DISKWRITELEN];
   long start; /* offset to first location to move to */
   long tomove; /* number of bytes to move */
   U16 numwritten;
   int success;

   success = FALSE;
   start = (long)offset * size;
   tomove = (long)count * size;
   if (debugflag == 10000)
      if (CheckBounds(start, tomove, handle))
         return success; /* out of bounds, don't do it */

   switch (handletable[handle].Nowhere.stored_at)
   {
   case NOWHERE: /* MoveToMemory */
      DisplayHandle(handle);
      break;

   case MEMORY: /* MoveToMemory */
#if defined(_WIN32)
	   _ASSERTE(handletable[handle].Linearmem.size >= size*count + start);
#endif
		memcpy(handletable[handle].Linearmem.memory + start, buffer, size*count);
		success = TRUE; /* No way to gauge success or failure */
		break;

   case DISK: /* MoveToMemory */
      rewind(handletable[handle].Disk.file);
      fseek(handletable[handle].Disk.file, start, SEEK_SET);
      while (tomove > DISKWRITELEN)
      {
         memcpy(diskbuf, buffer, (U16)DISKWRITELEN);
         numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
         if (numwritten != 1) {
            WhichDiskError(3);
            goto diskerror;
         }
         tomove -= DISKWRITELEN;
         buffer += DISKWRITELEN;
      }
      memcpy(diskbuf, buffer, (U16)tomove);
      numwritten = (U16)write1(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
      if (numwritten != 1) {
         WhichDiskError(3);
         break;
      }
      success = TRUE;
diskerror:
      break;
   } /* end of switch */
   if (!success && debugflag == 10000)
      DisplayHandle(handle);
   return success;
}

int MoveFromMemory(BYTE *buffer, U16 size, long count, long offset, U16 handle)
{ /* buffer points is the location to move the data to */
/* offset is the number of units from the beginning of buffer to start moving */
/* size is the size of the unit, count is the number of units to move */
/* Returns TRUE if successful, FALSE if failure */
   BYTE diskbuf[DISKWRITELEN];
   long start; /* first location to move */
   long tomove; /* number of bytes to move */
   U16 numread, i;
   int success;

   success = FALSE;
   start = (long)offset * size;
   tomove = (long)count * size;
   if (debugflag == 10000)
      if (CheckBounds(start, tomove, handle))
         return success; /* out of bounds, don't do it */

   switch (handletable[handle].Nowhere.stored_at)
   {
   case NOWHERE: /* MoveFromMemory */
      DisplayHandle(handle);
      break;

   case MEMORY: /* MoveFromMemory */
	   for (i = 0; i < count; i++)
	   {
		   memcpy(buffer, handletable[handle].Linearmem.memory + start, (U16) size);
		   start += size;
		   buffer += size;
	   }
      success = TRUE; /* No way to gauge success or failure */
      break;

   case DISK: /* MoveFromMemory */
      rewind(handletable[handle].Disk.file);
      fseek(handletable[handle].Disk.file, start, SEEK_SET);
      while (tomove > DISKWRITELEN)
      {
         numread = (U16)fread(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
         if (numread != 1 && !feof(handletable[handle].Disk.file)) {
            WhichDiskError(4);
            goto diskerror;
         }
         memcpy(buffer, diskbuf, (U16)DISKWRITELEN);
         tomove -= DISKWRITELEN;
         buffer += DISKWRITELEN;
      }
      numread = (U16)fread(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
      if (numread != 1 && !feof(handletable[handle].Disk.file)) {
         WhichDiskError(4);
         break;
      }
      memcpy(buffer, diskbuf, (U16)tomove);
      success = TRUE;
diskerror:
      break;
   } /* end of switch */
   if (!success && debugflag == 10000)
      DisplayHandle(handle);
   return success;
}

int SetMemory(int value, U16 size, long count, long offset, U16 handle)
{ /* value is the value to set memory to */
/* offset is the number of units from the start of allocated memory */
/* size is the size of the unit, count is the number of units to set */
/* Returns TRUE if successful, FALSE if failure */
   BYTE diskbuf[DISKWRITELEN];
   long start; /* first location to set */
   long tomove; /* number of bytes to set */
   U16 numwritten, i;
   int success;

   success = FALSE;
   start = (long)offset * size;
   tomove = (long)count * size;
   if (debugflag == 10000)
      if (CheckBounds(start, tomove, handle))
         return success; /* out of bounds, don't do it */

   switch (handletable[handle].Nowhere.stored_at)
   {
   case NOWHERE: /* SetMemory */
      DisplayHandle(handle);
      break;

   case MEMORY: /* SetMemory */
      for (i = 0; i < size; i++) {
         memset(handletable[handle].Linearmem.memory+start, value, (U16)count);
         start += count;
      }
      success = TRUE; /* No way to gauge success or failure */
      break;

   case DISK: /* SetMemory */
      memset(diskbuf, value, (U16)DISKWRITELEN);
      rewind(handletable[handle].Disk.file);
      fseek(handletable[handle].Disk.file, start, SEEK_SET);
      while (tomove > DISKWRITELEN)
      {
         numwritten = (U16)write1(diskbuf, (U16)DISKWRITELEN, 1, handletable[handle].Disk.file);
         if (numwritten != 1) {
            WhichDiskError(2);
            goto diskerror;
         }
         tomove -= DISKWRITELEN;
      }
      numwritten = (U16)write1(diskbuf, (U16)tomove, 1, handletable[handle].Disk.file);
      if (numwritten != 1) {
         WhichDiskError(2);
         break;
      }
      success = TRUE;
diskerror:
      break;
   } /* end of switch */
   if (!success && debugflag == 10000)
      DisplayHandle(handle);
   return success;
}

