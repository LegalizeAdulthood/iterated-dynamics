/*
**  UCLOCK.C
**
**  Contains routines to perform microsecond accuracy timing
**  operations.
**
**  Adapted from public domain source originally by David L. Fox
**  Modified by Bob Stout
*/

#include "uclock.h"

/* Constants */

#define CONTVAL   0x34    /* == 00110100 Control byte for 8253 timer.   */
                          /* Sets timer 0 to 2-byte read/write,         */
                          /* mode 2, binary.                            */
#define T0DATA    0x40    /* Timer 0 data port address.                 */
#define TMODE     0x43    /* Timer mode port address.                   */
#define BIOS_DS   0x40    /* BIOS data segment.                         */
#define B_TIKP    0x6c    /* Address of BIOS (18.2/s) tick count.       */
#define SCALE    10000    /* Scale factor for timer ticks.              */

/* The following values assume 18.2 BIOS ticks per second resulting from
   the 8253 being clocked at 1.19 MHz. */

#define us_BTIK  54925    /* Micro sec per BIOS clock tick.             */
#define f_BTIK    4595    /* Fractional part of usec per BIOS tick.     */
#define us_TTIK   8381    /* Usec per timer tick * SCALE. (4/4.77 MHz)  */

static int init = 0;

/*
**  usec_clock()
**
**  An analog of the clock() function, usec_clock() returns a number of
**  type uclock_t (defined in UCLOCK.H) which represents the number of
**  microseconds past midnight. Analogous to CLK_TCK is UCLK_TCK, the
**  number which a usec_clock() reading must be divided by to yield
**  a number of seconds.
*/

uclock_t usec_clock(void)
{
      unsigned char msb, lsb;
      unsigned int tim_ticks;
      static uclock_t last, init_count;
      static uclock_t far *c_ptr;
      uclock_t count, us_tmp;

      if (!init)
      {
            c_ptr = (uclock_t far *)MK_FP(BIOS_DS, B_TIKP);
            init  = 1;        /* First call, we have to set up timer.   */
            int_off(); 
            outp(TMODE, CONTVAL);   /* Write new control byte.          */
            outp(T0DATA, 0);        /* Initial count = 65636.           */
            outp(T0DATA, 0);
            init_count = *c_ptr;
            int_on();
            return 0;               /* First call returns zero.         */
      }

      /* Read PIT channel 0 count - see text                            */

      int_off();        /* Don't want an interrupt while getting time.  */
      outp(TMODE, 0);                           /* Latch count.         */
      lsb = (unsigned char)inp(T0DATA);         /* Read count.          */
      msb = (unsigned char)inp(T0DATA);

      /* Get BIOS tick count (read BIOS ram directly for speed and
         to avoid turning on interrupts).                               */

      count =  *c_ptr;
      int_on();                     /* Interrupts back on.              */
      if ((-1) == init)             /* Restart count                    */
      {
            init_count = count;
            init = 1;
      }

      /* Merge PIT channel 0 count with BIOS tick count                 */

      if (count < init_count)
            count += last;
      else  last = count;
      count -= init_count;
      tim_ticks = (unsigned)(-1) - ((msb << 8) | lsb);
      us_tmp    = count * us_BTIK;
      return (us_tmp + ((long)tim_ticks * us_TTIK + us_tmp % SCALE) / SCALE);
}

/*
**  restart_uclock()
**
**  Since usec_clock() bases its return value on a differential value,
**  a potential exists for problems in programs which run continuously
**  for more than 24 hours. In such an application, it's necessary, at
**  least once a day, to reset usec_clock's starting count.
*/

void restart_uclock(void)
{
      if (init)
            init = -1;
}
