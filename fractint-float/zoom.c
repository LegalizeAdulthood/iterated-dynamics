/*
    zoom.c - routines for zoombox manipulation and for panning

*/

#include <string.h>

  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"

#define PIXELROUND 0.00001

static void _fastcall zmo_calc(double, double, double *, double *, double);
static void _fastcall zmo_calcbf(bf_t,bf_t,bf_t,bf_t,bf_t,bf_t,bf_t,bf_t,bf_t);
static int  check_pan(void);
static void fix_worklist(void);
static void _fastcall move_row(int fromrow,int torow,int col);

/* big number declarations */
void calc_corner(bf_t target,bf_t p1,double p2,bf_t p3,double p4,bf_t p5)
{
   bf_t btmp1, btmp2 ,btmp3;
   int saved; saved = save_stack();
   btmp1 = alloc_stack(rbflength+2);
   btmp2 = alloc_stack(rbflength+2);
   btmp3 = alloc_stack(rbflength+2);

   /* use target as temporary variable */
   floattobf(btmp3, p2);
   mult_bf(btmp1,btmp3,p3);
   mult_bf(btmp2,floattobf(target, p4),p5);
   add_bf(target,btmp1,btmp2);
   add_a_bf(target,p1);
   restore_stack(saved);
}

int boxcolor;

#ifndef XFRACT
void dispbox(void)
{
   int i;
   int boxc = (colors-1)&boxcolor;
   unsigned char *values = (unsigned char *)boxvalues;
   int rgb[3];
   xorTARGA = 1;
   for(i=0;i<boxcount;i++)
   {
      if(istruecolor && truemode)
      {
         gettruecolor(boxx[i]-sxoffs,boxy[i]-syoffs,&rgb[0],&rgb[1],&rgb[2]);
         puttruecolor(boxx[i]-sxoffs,boxy[i]-syoffs,
                      rgb[0]^255,rgb[1]^255,rgb[2]^255);
      }
      else
         values[i] = (unsigned char)getcolor(boxx[i]-sxoffs,boxy[i]-syoffs);
   }
/* There is an interaction between getcolor and putcolor, so separate them */
   if (!(istruecolor && truemode)) /* don't need this for truecolor with truemode set */
      for(i=0;i<boxcount;i++)
      {
         if (colors == 2)
            putcolor(boxx[i]-sxoffs,boxy[i]-syoffs,(1 - values[i]));
         else
            putcolor(boxx[i]-sxoffs,boxy[i]-syoffs,boxc);
      }
   xorTARGA = 0;
}

void clearbox(void)
{
   int i;
   xorTARGA = 1;
   if(istruecolor && truemode)
   {
      dispbox();
   }
   else
   {
      unsigned char *values = (unsigned char *)boxvalues;
      for(i=0;i<boxcount;i++)
      {
         putcolor(boxx[i]-sxoffs,boxy[i]-syoffs,values[i]);
      }
   }
   xorTARGA = 0;
}
#endif

void drawbox(int drawit)
{
    struct coords tl,bl,tr,br; /* dot addr of topleft, botleft, etc */
    double tmpx,tmpy,dx,dy,rotcos,rotsin,ftemp1,ftemp2;
    double fxwidth,fxskew,fydepth,fyskew,fxadj;
    bf_t bffxwidth, bffxskew, bffydepth, bffyskew, bffxadj;
    int saved=0;
    if (zwidth==0) { /* no box to draw */
        if (boxcount!=0) { /* remove the old box from display */
            clearbox(); 
            boxcount = 0; }
        reset_zoom_corners();
        return; }
    if(bf_math)
    {
       saved = save_stack();
       bffxwidth = alloc_stack(rbflength+2);
       bffxskew  = alloc_stack(rbflength+2);
       bffydepth = alloc_stack(rbflength+2);
       bffyskew  = alloc_stack(rbflength+2);
       bffxadj   = alloc_stack(rbflength+2);
    }
    ftemp1 = PI*zrotate/72; /* convert to radians */
    rotcos = cos(ftemp1);   /* sin & cos of rotation */
    rotsin = sin(ftemp1);

    /* do some calcs just once here to reduce fp work a bit */
    fxwidth = sxmax-sx3rd;
    fxskew  = sx3rd-sxmin;
    fydepth = sy3rd-symax;
    fyskew  = symin-sy3rd;
    fxadj   = zwidth*zskew;

    if(bf_math)
    {
       /* do some calcs just once here to reduce fp work a bit */
       sub_bf(bffxwidth,bfsxmax,bfsx3rd);
       sub_bf(bffxskew,bfsx3rd,bfsxmin);
       sub_bf(bffydepth,bfsy3rd,bfsymax);
       sub_bf(bffyskew,bfsymin,bfsy3rd);
       floattobf(bffxadj, fxadj);
    }

    /* calc co-ords of topleft & botright corners of box */
    tmpx = zwidth/-2+fxadj; /* from zoombox center as origin, on xdots scale */
    tmpy = zdepth*finalaspectratio/2;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx; /* delta x to rotate topleft */
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy); /* delta y to rotate topleft */

    /* calc co-ords of topleft */
    ftemp1 = zbx + dx + fxadj;
    ftemp2 = zby + dy/finalaspectratio;

    tl.x   = (int)(ftemp1*(dxsize+PIXELROUND)); /* screen co-ords */
    tl.y   = (int)(ftemp2*(dysize+PIXELROUND));
    xxmin  = sxmin + ftemp1*fxwidth + ftemp2*fxskew; /* real co-ords */
    yymax  = symax + ftemp2*fydepth + ftemp1*fyskew;
    if(bf_math)
    {
       calc_corner(bfxmin,bfsxmin,ftemp1,bffxwidth,ftemp2,bffxskew);
       calc_corner(bfymax,bfsymax,ftemp2,bffydepth,ftemp1,bffyskew);
    }

    /* calc co-ords of bottom right */
    ftemp1 = zbx + zwidth - dx - fxadj;
    ftemp2 = zby - dy/finalaspectratio + zdepth;
    br.x   = (int)(ftemp1*(dxsize+PIXELROUND));
    br.y   = (int)(ftemp2*(dysize+PIXELROUND));
    xxmax  = sxmin + ftemp1*fxwidth + ftemp2*fxskew;
    yymin  = symax + ftemp2*fydepth + ftemp1*fyskew;
    if(bf_math)
    {
       calc_corner(bfxmax,bfsxmin,ftemp1,bffxwidth,ftemp2,bffxskew);
       calc_corner(bfymin,bfsymax,ftemp2,bffydepth,ftemp1,bffyskew);
    }
    /* do the same for botleft & topright */
    tmpx = zwidth/-2 - fxadj;
    tmpy = 0.0-tmpy;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx;
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy);
    ftemp1 = zbx + dx - fxadj;
    ftemp2 = zby + dy/finalaspectratio + zdepth;
    bl.x   = (int)(ftemp1*(dxsize+PIXELROUND));
    bl.y   = (int)(ftemp2*(dysize+PIXELROUND));
    xx3rd  = sxmin + ftemp1*fxwidth + ftemp2*fxskew;
    yy3rd  = symax + ftemp2*fydepth + ftemp1*fyskew;
    if(bf_math)
    {
       calc_corner(bfx3rd,bfsxmin,ftemp1,bffxwidth,ftemp2,bffxskew);
       calc_corner(bfy3rd,bfsymax,ftemp2,bffydepth,ftemp1,bffyskew);
       restore_stack(saved);
    }
    ftemp1 = zbx + zwidth - dx + fxadj;
    ftemp2 = zby - dy/finalaspectratio;
    tr.x   = (int)(ftemp1*(dxsize+PIXELROUND));
    tr.y   = (int)(ftemp2*(dysize+PIXELROUND));

    if (boxcount!=0) { /* remove the old box from display */
        clearbox(); 
        boxcount = 0; }

    if (drawit) { /* caller wants box drawn as well as co-ords calc'd */
#ifndef XFRACT
        /* build the list of zoom box pixels */
        addbox(tl); addbox(tr);               /* corner pixels */
        addbox(bl); addbox(br);
        drawlines(tl,tr,bl.x-tl.x,bl.y-tl.y); /* top & bottom lines */
        drawlines(tl,bl,tr.x-tl.x,tr.y-tl.y); /* left & right lines */
#else
        boxx[0] = tl.x + sxoffs;
        boxy[0] = tl.y + syoffs;
        boxx[1] = tr.x + sxoffs;
        boxy[1] = tr.y + syoffs;
        boxx[2] = br.x + sxoffs;
        boxy[2] = br.y + syoffs;
        boxx[3] = bl.x + sxoffs;
        boxy[3] = bl.y + syoffs;
        boxcount = 1;
#endif
        dispbox();
        }
    }

void _fastcall drawlines(struct coords fr, struct coords to,
                                int dx, int dy)
{   int xincr,yincr,ctr;
    int altctr,altdec,altinc;
    struct coords tmpp,line1,line2;

    if (abs(to.x-fr.x) > abs(to.y-fr.y)) { /* delta.x > delta.y */
        if (fr.x>to.x) { /* swap so from.x is < to.x */
            tmpp = fr; fr = to; to = tmpp; }
        xincr = (to.x-fr.x)*4/sxdots+1; /* do every 1st, 2nd, 3rd, or 4th dot */
        ctr = (to.x-fr.x-1)/xincr;
        altdec = abs(to.y-fr.y)*xincr;
        altinc = to.x-fr.x;
        altctr = altinc/2;
        yincr = (to.y>fr.y)?1:-1;
        line2.x = (line1.x = fr.x) + dx;
        line2.y = (line1.y = fr.y) + dy;
        while (--ctr>=0) {
            line1.x += xincr;
            line2.x += xincr;
            altctr -= altdec;
            while (altctr<0) {
                altctr  += altinc;
                line1.y += yincr;
                line2.y += yincr;
                }
            addbox(line1);
            addbox(line2);
            }
        }

    else { /* delta.y > delta.x */
        if (fr.y>to.y) { /* swap so from.y is < to.y */
            tmpp = fr; fr = to; to = tmpp; }
        yincr = (to.y-fr.y)*4/sydots+1; /* do every 1st, 2nd, 3rd, or 4th dot */
        ctr = (to.y-fr.y-1)/yincr;
        altdec = abs(to.x-fr.x)*yincr;
        altinc = to.y-fr.y;
        altctr = altinc/2;
        xincr = (to.x>fr.x) ? 1 : -1;
        line2.x = (line1.x = fr.x) + dx;
        line2.y = (line1.y = fr.y) + dy;
        while (--ctr>=0) {
            line1.y += yincr;
            line2.y += yincr;
            altctr  -= altdec;
            while (altctr<0) {
                altctr  += altinc;
                line1.x += xincr;
                line2.x += xincr;
                }
            addbox(line1);
            addbox(line2);
            }
        }
    }

void _fastcall addbox(struct coords point)
{
    point.x += sxoffs;
    point.y += syoffs;
    if (point.x >= 0 && point.x < sxdots && 
        point.y >= 0 && point.y < sydots) {
        boxx[boxcount] = point.x;
        boxy[boxcount] = point.y;
        ++boxcount;
        }
    }

void moveboxf(double dx, double dy)
{   int align,row,col;
    align = check_pan();
    if (dx!=0.0) {
        if ((zbx += dx) + zwidth/2 < 0)  /* center must stay onscreen */
            zbx = zwidth/-2;
        if (zbx + zwidth/2 > 1)
            zbx = 1.0 - zwidth/2;
        if (align != 0
          && ((col = (int)(zbx*(dxsize+PIXELROUND))) & (align-1)) != 0) {
            if (dx > 0) col += align;
            col -= col & (align-1); /* adjust col to pass alignment */
            zbx = (double)col/dxsize; }
        }
    if (dy!=0.0) {
        if ((zby += dy) + zdepth/2 < 0)
            zby = zdepth/-2;
        if (zby + zdepth/2 > 1)
            zby = 1.0 - zdepth/2;
        if (align != 0
          && ((row = (int)(zby*(dysize+PIXELROUND))) & (align-1)) != 0) {
            if (dy > 0) row += align;
            row -= row & (align-1);
            zby = (double)row/dysize; }
        }
#ifndef XFRACT
    if (video_scroll != 0) {  /* scroll screen center to the box center */
        col = (int)((zbx + zwidth/2)*(dxsize + PIXELROUND)) + sxoffs;
        row = (int)((zby + zdepth/2)*(dysize + PIXELROUND)) + syoffs;
        switch (zscroll) {
            case 0:  /* fixed - screen center fixed to the zoombox center */
                scroll_center(col,row);
                break;
            case 1:  /* relaxed - as the zoombox center leaves the screen */
                if ((col -= video_startx) > 0 && (col -= vesa_xres - 1) < 0)
                    col = 0;
                if ((row -= video_starty) > 0 && (row -= vesa_yres - 1) < 0)
                    row = 0;
                if (col != 0 || row != 0)
                    scroll_relative(col, row);
                break;
            }
        }
#endif
    }

static void _fastcall chgboxf(double dwidth, double ddepth)
{
    if (zwidth+dwidth > 1)
        dwidth = 1.0-zwidth;
    if (zwidth+dwidth < 0.05)
        dwidth = 0.05-zwidth;
    zwidth += dwidth;
    if (zdepth+ddepth > 1)
        ddepth = 1.0-zdepth;
    if (zdepth+ddepth < 0.05)
        ddepth = 0.05-zdepth;
    zdepth += ddepth;
    moveboxf(dwidth/-2,ddepth/-2); /* keep it centered & check limits */
    }

void resizebox(int steps)
{
    double deltax,deltay;
    if (zdepth*screenaspect > zwidth) { /* box larger on y axis */
        deltay = steps * 0.036 / screenaspect;
        deltax = zwidth * deltay / zdepth;
        }
    else {                              /* box larger on x axis */
        deltax = steps * 0.036;
        deltay = zdepth * deltax / zwidth;
        }
    chgboxf(deltax,deltay);
    }

void chgboxi(int dw, int dd)
{   /* change size by pixels */
    chgboxf( (double)dw/dxsize, (double)dd/dysize );
    }
#ifdef C6
#pragma optimize("e",off)  /* MSC 6.00A messes up next rtn with "e" on */
#endif

extern void show_three_bf();

static void _fastcall zmo_calcbf(bf_t bfdx, bf_t bfdy,
    bf_t bfnewx, bf_t bfnewy,bf_t bfplotmx1, bf_t bfplotmx2, bf_t bfplotmy1,
    bf_t bfplotmy2, bf_t bfftemp)
{
    bf_t btmp1, btmp2, btmp3, btmp4, btempx, btempy ;
    bf_t btmp2a, btmp4a;
    int saved; saved = save_stack();

    btmp1  = alloc_stack(rbflength+2);
    btmp2  = alloc_stack(rbflength+2);
    btmp3  = alloc_stack(rbflength+2);
    btmp4  = alloc_stack(rbflength+2);
    btmp2a = alloc_stack(rbflength+2);
    btmp4a = alloc_stack(rbflength+2);
    btempx = alloc_stack(rbflength+2);
    btempy = alloc_stack(rbflength+2);

    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */

    /* tempx = dy * plotmx1 - dx * plotmx2; */
    mult_bf(btmp1,bfdy,bfplotmx1);
    mult_bf(btmp2,bfdx,bfplotmx2);
    sub_bf(btempx,btmp1,btmp2);

    /* tempy = dx * plotmy1 - dy * plotmy2; */
    mult_bf(btmp1,bfdx,bfplotmy1);
    mult_bf(btmp2,bfdy,bfplotmy2);
    sub_bf(btempy,btmp1,btmp2);

    /* calc new corner by extending from current screen corners */
    /* *newx = sxmin + tempx*(sxmax-sx3rd)/ftemp + tempy*(sx3rd-sxmin)/ftemp; */
    sub_bf(btmp1,bfsxmax,bfsx3rd);
    mult_bf(btmp2,btempx,btmp1);
    /* show_three_bf("fact1",btempx,"fact2",btmp1,"prod ",btmp2,70); */
    div_bf(btmp2a,btmp2,bfftemp);
    /* show_three_bf("num  ",btmp2,"denom",bfftemp,"quot ",btmp2a,70); */
    sub_bf(btmp3,bfsx3rd,bfsxmin);
    mult_bf(btmp4,btempy,btmp3);
    div_bf(btmp4a,btmp4,bfftemp);
    add_bf(bfnewx,bfsxmin,btmp2a);
    add_a_bf(bfnewx,btmp4a);

    /* *newy = symax + tempy*(sy3rd-symax)/ftemp + tempx*(symin-sy3rd)/ftemp; */
    sub_bf(btmp1,bfsy3rd,bfsymax);
    mult_bf(btmp2,btempy,btmp1);
    div_bf(btmp2a,btmp2,bfftemp);
    sub_bf(btmp3,bfsymin,bfsy3rd);
    mult_bf(btmp4,btempx,btmp3);
    div_bf(btmp4a,btmp4,bfftemp);
    add_bf(bfnewy,bfsymax,btmp2a);
    add_a_bf(bfnewy,btmp4a);
    restore_stack(saved);
}

static void _fastcall zmo_calc(double dx, double dy, double *newx, double *newy, double ftemp)
{
    double tempx,tempy;
    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */
    tempx = dy * plotmx1 - dx * plotmx2;
    tempy = dx * plotmy1 - dy * plotmy2;

    /* calc new corner by extending from current screen corners */
    *newx = sxmin + tempx*(sxmax-sx3rd)/ftemp + tempy*(sx3rd-sxmin)/ftemp;
    *newy = symax + tempy*(sy3rd-symax)/ftemp + tempx*(symin-sy3rd)/ftemp;
}

void zoomoutbf(void) /* for ctl-enter, calc corners for zooming out */
{
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    bf_t savbfxmin,savbfymax,bfftemp;
    bf_t tmp1, tmp2, tmp3, tmp4, tmp5, tmp6,bfplotmx1,bfplotmx2,bfplotmy1,bfplotmy2;
    int saved;
    saved = save_stack();
    savbfxmin = alloc_stack(rbflength+2);
    savbfymax = alloc_stack(rbflength+2);
    bfftemp   = alloc_stack(rbflength+2);
    tmp1      = alloc_stack(rbflength+2);
    tmp2      = alloc_stack(rbflength+2);
    tmp3      = alloc_stack(rbflength+2);
    tmp4      = alloc_stack(rbflength+2);
    tmp5      = alloc_stack(rbflength+2);
    tmp6      = alloc_stack(rbflength+2);
    bfplotmx1 = alloc_stack(rbflength+2);
    bfplotmx2 = alloc_stack(rbflength+2);
    bfplotmy1 = alloc_stack(rbflength+2);
    bfplotmy2 = alloc_stack(rbflength+2);
    /* ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax); */
    sub_bf(tmp1,bfymin,bfy3rd);
    sub_bf(tmp2,bfx3rd,bfxmin);
    sub_bf(tmp3,bfxmax,bfx3rd);
    sub_bf(tmp4,bfy3rd,bfymax);
    mult_bf(tmp5,tmp1,tmp2);
    mult_bf(tmp6,tmp3,tmp4);
    sub_bf(bfftemp,tmp5,tmp6);
    /* plotmx1 = (xx3rd-xxmin); */ ; /* reuse the plotxxx vars is safe */
    copy_bf(bfplotmx1,tmp2);
    /* plotmx2 = (yy3rd-yymax); */
    copy_bf(bfplotmx2,tmp4);
    /* plotmy1 = (yymin-yy3rd); */
    copy_bf(bfplotmy1,tmp1);
    /* plotmy2 = (xxmax-xx3rd); */;
    copy_bf(bfplotmy2,tmp3);

    /* savxxmin = xxmin; savyymax = yymax; */
    copy_bf(savbfxmin,bfxmin); copy_bf(savbfymax,bfymax);

    sub_bf(tmp1,bfsxmin,savbfxmin); sub_bf(tmp2,bfsymax,savbfymax);
    zmo_calcbf(tmp1,tmp2,bfxmin,bfymax,bfplotmx1,bfplotmx2,bfplotmy1,
               bfplotmy2,bfftemp);
    sub_bf(tmp1,bfsxmax,savbfxmin); sub_bf(tmp2,bfsymin,savbfymax);
    zmo_calcbf(tmp1,tmp2,bfxmax,bfymin,bfplotmx1,bfplotmx2,bfplotmy1,
               bfplotmy2,bfftemp);
    sub_bf(tmp1,bfsx3rd,savbfxmin); sub_bf(tmp2,bfsy3rd,savbfymax);
    zmo_calcbf(tmp1,tmp2,bfx3rd,bfy3rd,bfplotmx1,bfplotmx2,bfplotmy1,
               bfplotmy2,bfftemp);
    restore_stack(saved);
}

void zoomoutdbl(void) /* for ctl-enter, calc corners for zooming out */
{
   /* (xxmin,yymax), etc, are already set to zoombox corners;
      (sxmin,symax), etc, are still the screen's corners;
      use the same logic as plot_orbit stuff to first calculate current screen
      corners relative to the zoombox, as if the zoombox were a square with
      upper left (0,0) and width/depth 1; ie calc the current screen corners
      as if plotting them from the zoombox;
      then extend these co-ords from current real screen corners to get
      new actual corners
      */
   double savxxmin,savyymax,ftemp;
   ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
   plotmx1 = (xx3rd-xxmin); /* reuse the plotxxx vars is safe */
   plotmx2 = (yy3rd-yymax);
   plotmy1 = (yymin-yy3rd);
   plotmy2 = (xxmax-xx3rd);
   savxxmin = xxmin; savyymax = yymax;
   zmo_calc(sxmin-savxxmin,symax-savyymax,&xxmin,&yymax,ftemp);
   zmo_calc(sxmax-savxxmin,symin-savyymax,&xxmax,&yymin,ftemp);
   zmo_calc(sx3rd-savxxmin,sy3rd-savyymax,&xx3rd,&yy3rd,ftemp);
}

void zoomout(void) /* for ctl-enter, calc corners for zooming out */
{
   if(bf_math)
   {
      zoomoutbf();
   }
   else
      zoomoutdbl();
}

#ifdef C6
#pragma optimize("e",on)  /* back to normal */
#endif

void aspectratio_crop(float oldaspect,float newaspect)
{
   double ftemp,xmargin,ymargin;
   if (newaspect > oldaspect) { /* new ratio is taller, crop x */
      ftemp = (1.0 - oldaspect / newaspect) / 2;
      xmargin = (xxmax - xx3rd) * ftemp;
      ymargin = (yymin - yy3rd) * ftemp;
      xx3rd += xmargin;
      yy3rd += ymargin;
      }
   else                       { /* new ratio is wider, crop y */
      ftemp = (1.0 - newaspect / oldaspect) / 2;
      xmargin = (xx3rd - xxmin) * ftemp;
      ymargin = (yy3rd - yymax) * ftemp;
      xx3rd -= xmargin;
      yy3rd -= ymargin;
      }
   xxmin += xmargin;
   yymax += ymargin;
   xxmax -= xmargin;
   yymin -= ymargin;
}

static int check_pan(void) /* return 0 if can't, alignment requirement if can */
{   int i,j;
    if ((calc_status != 2 && calc_status != 4) || evolving)
        return(0); /* not resumable, not complete */
    if ( curfractalspecific->calctype != StandardFractal
      && curfractalspecific->calctype != calcmandfp
      && curfractalspecific->calctype != lyapunov
      && curfractalspecific->calctype != calcfroth)
        return(0); /* not a worklist-driven type */
    if (zwidth != 1.0 || zdepth != 1.0 || zskew != 0.0 || zrotate != 0.0)
        return(0); /* not a full size unrotated unskewed zoombox */
    /* can pan if we get this far */
    if (calc_status == 4)
        return(1); /* image completed, align on any pixel */
    if (potflag && pot16bit)
        return(1); /* 1 pass forced so align on any pixel */
    if (stdcalcmode == 'b')
        return(1); /* btm, align on any pixel */
    if (stdcalcmode == 't')
        return(0); /* tesselate, can't do it */
    if (stdcalcmode == 'd')
	return(0); /* diffusion scan: can't do it either */
    if (stdcalcmode != 'g' || (curfractalspecific->flags&NOGUESS)) {
        if (stdcalcmode == '2' || stdcalcmode == '3') /* align on even pixel for 2pass */
           return(2);
        return(1); /* assume 1pass */
        }
    /* solid guessing */
    start_resume();
    get_resume(sizeof(num_worklist),&num_worklist,sizeof(worklist),worklist,0);
    /* don't do end_resume! we're just looking */
    i = 9;
    for (j=0; j<num_worklist; ++j) /* find lowest pass in any pending window */
        if (worklist[j].pass < i)
            i = worklist[j].pass;
    j = ssg_blocksize(); /* worst-case alignment requirement */
    while (--i >= 0)
        j = j>>1; /* reduce requirement */
    return(j);
    }

static void _fastcall move_row(int fromrow,int torow,int col)
/* move a row on the screen */
{   int startcol,endcol,tocol;
    memset(dstack,0,xdots); /* use dstack as a temp for the row; clear it */
    if (fromrow >= 0 && fromrow < ydots) {
        tocol = startcol = 0;
        endcol = xdots-1;
        if (col < 0) {
            tocol -= col;
            endcol += col; }
        if (col > 0)
            startcol += col;
        get_line(fromrow,startcol,endcol,(BYTE *)&dstack[tocol]);
        }
    put_line(torow,0,xdots-1,(BYTE *)dstack);
    }

int init_pan_or_recalc(int do_zoomout) /* decide to recalc, or to chg worklist & pan */
{   int i,j,row,col,y,alignmask,listfull;
    if (zwidth == 0.0)
        return(0); /* no zoombox, leave calc_status as is */
    /* got a zoombox */
    if ((alignmask=check_pan()-1) < 0 || evolving) {
        calc_status = 0; /* can't pan, trigger recalc */
        return(0); }
    if (zbx == 0.0 && zby == 0.0) {
        clearbox();
        return(0); } /* box is full screen, leave calc_status as is */
    col = (int)(zbx*(dxsize+PIXELROUND)); /* calc dest col,row of topleft pixel */
    row = (int)(zby*(dysize+PIXELROUND));
    if (do_zoomout) { /* invert row and col */
        row = 0-row;
        col = 0-col; }
    if ((row&alignmask) != 0 || (col&alignmask) != 0) {
        calc_status = 0; /* not on useable pixel alignment, trigger recalc */
        return(0); }
    /* pan */
    num_worklist = 0;
    if (calc_status == 2) {
       start_resume();
       get_resume(sizeof(num_worklist),&num_worklist,sizeof(worklist),worklist,0);
       } /* don't do end_resume! we might still change our mind */
    /* adjust existing worklist entries */
    for (i=0; i<num_worklist; ++i) {
        worklist[i].yystart -= row;
        worklist[i].yystop  -= row;
        worklist[i].yybegin -= row;
        worklist[i].xxstart -= col;
        worklist[i].xxstop  -= col;
        worklist[i].xxbegin -= col;
        }
    /* add worklist entries for the new edges */
    listfull = i = 0;
    j = ydots-1;
    if (row < 0) {
        listfull |= add_worklist(0,xdots-1,0,0,0-row-1,0,0,0);
        i = 0 - row; }
    if (row > 0) {
        listfull |= add_worklist(0,xdots-1,0,ydots-row,ydots-1,ydots-row,0,0);
        j = ydots - row - 1; }
    if (col < 0)
        listfull |= add_worklist(0,0-col-1,0,i,j,i,0,0);
    if (col > 0)
        listfull |= add_worklist(xdots-col,xdots-1,xdots-col,i,j,i,0,0);
    if (listfull != 0) {
    static FCODE msg[] = {"\
Tables full, can't pan current image.\n\
Cancel resumes old image, continue pans and calculates a new one."};
        if (stopmsg(2,msg)) {
            zwidth = 0; /* cancel the zoombox */
            drawbox(1); }
        else
            calc_status = 0; /* trigger recalc */
        return(0); }
    /* now we're committed */
    calc_status = 2;
    clearbox();
    if (row > 0) /* move image up */
        for (y=0; y<ydots; ++y) move_row(y+row,y,col);
    else         /* move image down */
        for (y=ydots; --y>=0;)  move_row(y+row,y,col);
    fix_worklist(); /* fixup any out of bounds worklist entries */
    alloc_resume(sizeof(worklist)+20,2); /* post the new worklist */
    put_resume(sizeof(num_worklist),&num_worklist,sizeof(worklist),worklist,0);
    return(0);
    }

static void _fastcall restart_window(int wknum)
/* force a worklist entry to restart */
{   int yfrom,yto,xfrom,xto;
    if ((yfrom = worklist[wknum].yystart) < 0) yfrom = 0;
    if ((xfrom = worklist[wknum].xxstart) < 0) xfrom = 0;
    if ((yto = worklist[wknum].yystop) >= ydots) yto = ydots - 1;
    if ((xto = worklist[wknum].xxstop) >= xdots) xto = xdots - 1;
    memset(dstack,0,xdots); /* use dstack as a temp for the row; clear it */
    while (yfrom <= yto)
        put_line(yfrom++,xfrom,xto,(BYTE *)dstack);
    worklist[wknum].sym = worklist[wknum].pass = 0;
    worklist[wknum].yybegin = worklist[wknum].yystart;
    worklist[wknum].xxbegin = worklist[wknum].xxstart;
}

static void fix_worklist(void) /* fix out of bounds and symmetry related stuff */
{   int i,j,k;
    WORKLIST *wk;
    for (i=0; i<num_worklist; ++i) {
        wk = &worklist[i];
        if ( wk->yystart >= ydots || wk->yystop < 0
          || wk->xxstart >= xdots || wk->xxstop < 0) { /* offscreen, delete */
            for (j=i+1; j<num_worklist; ++j)
                worklist[j-1] = worklist[j];
            --num_worklist;
            --i;
            continue; }
        if (wk->yystart < 0) { /* partly off top edge */
            if ((wk->sym&1) == 0) { /* no sym, easy */
                wk->yystart = 0;
                wk->xxbegin = 0; }
            else { /* xaxis symmetry */
                if ((j = wk->yystop + wk->yystart) > 0
                  && num_worklist < MAXCALCWORK) { /* split the sym part */
                    worklist[num_worklist] = worklist[i];
                    worklist[num_worklist].yystart = 0;
                    worklist[num_worklist++].yystop = j;
                    wk->yystart = j+1; }
                else
                    wk->yystart = 0;
                restart_window(i); /* restart the no-longer sym part */
            }
        }
        if (wk->yystop >= ydots) { /* partly off bottom edge */
           j = ydots-1;
           if ((wk->sym&1) != 0) { /* uses xaxis symmetry */
              if ((k = wk->yystart + (wk->yystop - j)) < j) {
                 if (num_worklist >= MAXCALCWORK) /* no room to split */
                    restart_window(i);
                 else { /* split it */
                    worklist[num_worklist] = worklist[i];
                    worklist[num_worklist].yystart = k;
                    worklist[num_worklist++].yystop = j;
                    j = k-1; }
              }
              wk->sym &= -1 - 1; }
           wk->yystop = j; }
        if (wk->xxstart < 0) { /* partly off left edge */
            if ((wk->sym&2) == 0) /* no sym, easy */
                wk->xxstart = 0;
            else { /* yaxis symmetry */
                if ((j = wk->xxstop + wk->xxstart) > 0
                  && num_worklist < MAXCALCWORK) { /* split the sym part */
                    worklist[num_worklist] = worklist[i];
                    worklist[num_worklist].xxstart = 0;
                    worklist[num_worklist++].xxstop = j;
                    wk->xxstart = j+1; }
                else
                    wk->xxstart = 0;
                restart_window(i); /* restart the no-longer sym part */
            }
        }
        if (wk->xxstop >= xdots) { /* partly off right edge */
           j = xdots-1;
           if ((wk->sym&2) != 0) { /* uses xaxis symmetry */
              if ((k = wk->xxstart + (wk->xxstop - j)) < j) {
                 if (num_worklist >= MAXCALCWORK) /* no room to split */
                    restart_window(i);
                 else { /* split it */
                    worklist[num_worklist] = worklist[i];
                    worklist[num_worklist].xxstart = k;
                    worklist[num_worklist++].xxstop = j;
                    j = k-1; }
              }
              wk->sym &= -1 - 2; }
           wk->xxstop = j; }
        if (wk->yybegin < wk->yystart) wk->yybegin = wk->yystart;
        if (wk->yybegin > wk->yystop)  wk->yybegin = wk->yystop;
        if (wk->xxbegin < wk->xxstart) wk->xxbegin = wk->xxstart;
        if (wk->xxbegin > wk->xxstop)  wk->xxbegin = wk->xxstop;
        }
    tidy_worklist(); /* combine where possible, re-sort */
}
