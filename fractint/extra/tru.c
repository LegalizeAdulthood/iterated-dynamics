/* 
  Fractint writes the escape iteration of fractal to a 24 bit Targa file
  called ITERATES.TGA. This program reads that file and recovers the
  iteration value. You can write your own truecolor mapping algorithm. It
  should go in the routine rgbmap().
*/

#include <stdio.h>

static long maxi = -1;

void rgbmap(long maxiter, long iter,
   unsigned char *r, unsigned char *g, unsigned char *b)
{
   if (iter > maxi)
      maxi = iter;
   *b = iter & 0xff;
   *g = (iter >> 8) & 0xff;
   *r = (iter >> 16) & 0xff;
}

main()
{
   int xdots, ydots, i, j, err;
   long iter, maxiter;
   char red, green, blue;
   FILE *fpin, *fpout;
   char buf1[12];
   char buf2[2];

   if ((fpin = fopen("iterates.tga","rb")) == NULL)
   {
      fprintf(stderr,"Can't open flat.out\n");
      exit(1);
   }
   if ((fpout = fopen("new.tga","wb")) == NULL)
   {
      fprintf(stderr,"Can't open new.tga\n");
      exit(1);
   }

   fread (buf1,   12,1,fpin);    /* read 12 bytes */
   fread (&xdots,  2,1,fpin);    /* read xdots */
   fread (&ydots,  2,1,fpin);    /* read ydots */
   fread (buf2,    2,1,fpin);    /* read 2 bytes */
   fread (&maxiter,4,1,fpin);    /* read 4 bytes */
   buf1[0] = 0; /* were not writing maxiter */
   fwrite (buf1,  12,1,fpout);   /* write 12 bytes */
   fwrite (&xdots, 2,1,fpout);   /* write xdots */
   fwrite (&ydots, 2,1,fpout);   /* write ydots */
   fwrite (buf2,   2,1,fpout);   /* write 2 bytes */

   printf("xdots %d ydots %d maxiter %ld\n",xdots,ydots,maxiter);

   for (j = 0; j < ydots; j++)
   {
//      printf("row %2d maxi %6ld   \r",j,maxi);
      for (i = 0; i < xdots; i++)
      {
         iter = 0;
         if ((err=fread(&iter,3,1,fpin)) == 0)
         {
            printf("err at row %d col %d\n",j,i);
            exit(1);
         }   
         printf("row %2d col %2d iter %8ld          \n",j,i,iter);
         /* map iterations to colors */
         rgbmap(maxiter,iter,&red,&green,&blue);
         fwrite(&blue, 1,1,fpout);
         fwrite(&green,1,1,fpout);
         fwrite(&red,  1,1,fpout);
      }
   }
}

