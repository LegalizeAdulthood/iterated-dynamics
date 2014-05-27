  { ======================================================================== }
  {         File originally distributed with FRAC'Cetera Vol 2 Iss 7         }
  {==========================================================================
   =  Compiled by Jon Horner for FRAC'Cetera from several sources - Jul 93  =
   =  F'names, where present, represent FRAC'Cetera created variations or   =
   =  derivatives based, often quite loosely, on the author's originals.    =
   ==========================================================================}

{ IKENAGA - Formula originally discovered by Bruce Ikenaga, at Western Reserve
  University, Indiana.  Documented in Dewdney's `Armchair Universe".

          The Ikenaga set is:    Z(n+1) =Z(n)^3 + (C-1) * Z(n) - C
          where:                 Z(n) = x + yi and C = a + bi

  In Basic (with Mandelbrot for comparison):

   Mandelbrot:               Ikenaga:

   newX = x*x - y*y + a      newX = x*x*x - 3*x*y*y + a*x - b*y - x - a
   newY = 2*x*y     + b      newY = 3*x*x*y - y*y*y + a*y + b*x - y - b

  Ikenaga is also in R.F.J. Stewart's MANDPLOT; Larry Cobb's DRAGONS4;
  Mike Curnow's !Fractal for the Archimedes, and Tim Harris's CAL.

   ==========================================================================}

!_Press_F2_!     {; There is text in this formula file.  Shell to DOS with
                  ; the <d> key and use a text reader to browse the file.
                 }

Ikenaga(XAXIS)      { ; this version correct per Roderick Stewart -
                      ; announcement in Fractal Report 25 p2.
                      ; same as letter, Joyce Haslam Mar 1993
                      ; z=z*z*z+ (c-1)*z-c produces same results.
                    z = (0,0), c = pixel :
                    z = z * z * z + z * (c-1) - c ,
                    |z|<=4
 }

IkenagaJ.1(XAXIS)      { ; this version correct per Roderick Stewart -
                      ; announcement in Fractal Report 25 p2.
                      ; same as letter, Joyce Haslam Mar 1993
                      ; z=z*z*z+ (c-1)*z-c produces same results.
                    z = (0,0), c = pixel :
                    z = z * (z * z + (c-1)) - c ,
                    |z|<=4
 }

IkenagaJUL          { ; formula from a letter from Joyce Haslam Mar 1993.
                      ; Asymmetric.   try p1 = (0.56667, 0.36)
                      ; Next line, from Haslam article Fractal Report 24 p5
                      ; z=z*z*z+ (c-1)*z-c produces same results.
                      ; Same as Julike in REB001.FRM
                    z = pixel, c = p1 :
                    z = z * z * z + z * (c-1) - c ,
                    |z| <= 4
 }

IkenagaJULJ.1          { ; formula from a letter from Joyce Haslam Mar 1993.
                      ; Asymmetric.   try p1 = (0.56667, 0.36)
                      ; Next line, from Haslam article Fractal Report 24 p5
                      ; z=z*z*z+ (c-1)*z-c produces same results.
                      ; Same as Julike in REB001.FRM
                    z = pixel, c = p1 :
                    z = z * (z * z + (c-1)) - c ,
                    |z| <= 4
 }

{ The rest are all variations on the two basic formulas }

IkenagaPwr(XAXIS)   { ; from Jon Horner
                    z = (0,0), c = pixel :
                    z = z * z * z + z * (c-1) - c ^ p1 ,
                    |z| <=4
}

IkenagaPwrJul       { ; from Jon Horner - asymmetric
                      ; try p1 = (0.035, -0.35), p2 = 2
                    z = pixel, c = p1 :
                    z = z * z * z + z * (c-1) - c ^ p2 ,
                    |z| <=4
}

Ikenaga4(XAXIS)     { ; CAL v3.8 calls it Ikenaga-4Set
                      ; Per Haslam, Fractal Report 25 p2 (IKE4)
                      ; z^4 is used instead of z^3 and multiplication
                      ; is used in place of addition.
                    z = (0,0), c = pixel:
                    z = z * z * z * z * (c-1) - c ,
                    |z|<= 4
 }

Ikenaga4JUL(ORIGIN) { ; Jon Horner, from IKE4 - Fractal Report 25 p2
                      ; try p1 = (0.3874, 0.85)
                    z = pixel, c = p1 :
                    z = z * z * z * z * (c-1) - c,
                    |z| <= 4
 }

IkenagaABS(XAXIS)   { ; Jon Horner
                      ; Ikenaga with alternative bailout
                    z = (0,0), c = pixel :
                    z = z * z * z + z * (c-1) - c ,
                    abs(z)<=4
 }

IkenagaABSJUL       { ; Jon Horner
                      ; IkenagaJul with alternative bailout
                    z = pixel, c = p1 :
                    z = z * z * z + z * (c-1) - c ,
                    abs(z) <= 4
 }

Ikenaga4BIO(XAXIS)  { ; Ikenaga4 variation - Jon Horner
                      ; float=y
                    z = (0,0), c = pixel :
                    z = z * z * z * z * (c-1) - c ,
                    |real(z)|<=4 || |imag(z)|<=4
 }

Ikenaga4BIOJUL(ORIGIN){ ; Jon Horner, from IKE4 - FR 25, p2
                    ; try p1 = (0.3874, 0.85)  float=y
                  z = pixel, c = p1 :
                  z = z * z * z * z * (c-1) - c,
                  |real(z)|<=4 || |imag(z)|<=4
 }

IkenagaFN(XAXIS)    {; Jon Horner
                     ; derived from Ikenaga
                    z = (0,0), c = fn1(pixel) :
                    z = z * z * z + z * (c-1) - c ,
                    |z| <=4 }

IkenagaFNJUL        { ; Jon Horner
                      ; derived from IkenagaJul
                      ; asymmetric:   try p1 = (0.56667, 0.36)
                    z = fn1(pixel), c = p1 :
                    z = z * z * z + z * (c-1) - c,
                    |z| <= 4
 }

IkenagaJUL1+(ORIGIN){ ; formula from an article by Joyce Haslam
                      ; in Fractal Report 24 (w/+pixel stead of +c)
                      ; symmetric
                 z = pixel, c = p1 :
                 z = z * z * z + (c-1) * z + pixel ,
                 |z|<=4
 }

Ikenaga2(XAXIS)     { ; from Joyce Haslam article, Fractal Report Iss 24 p5.
                      ; Uses pixel instead of c !!!!
                      ; CAL v4.0 calls it UnamiSet. FR 25 calls it Unknown.
                      ; CAL v4.0 IkenagaSet(XAXIS) =
                      ; {z=0:z=z*z*z+z*(pixel-1)-pixel,|z|<=|2|
                    z = pixel:
                    z = z * z * z + (pixel-1) * z - pixel,
                    |z|<=4
 }                  

IkenagaMap(XAXIS)   {; from REB001.FRM - by Ron Barnett 70153,1233
                     ; The initial starting point allows the function to provide
                     ; a "map" for the corresponding Julia (IkenagaJul)
                    z = ((1-pixel)/3)^0.5:
                    z = z*z*z + (pixel-1)*z - pixel,
                    |z| <= 4
 }

IkeNewtMand         { ; from REB001.FRM - by Ron Barnett 70153,1233
                      ; p1 > 0, < 2, float=yes
                    z = c = pixel:
                    zf = z*z*z + (c-1)*z - c;
                    zd = 3*z*z + c-1;
                    z = z - p1*zf/zd,
                    0.001 <= |zf|
 }

IkeNewtJul          { ; from REB001.FRM - by Ron Barnett 70153,1233
                      ; p1 > 0, < 2, float=yes
                    z =  pixel:
                    zf = z*z*z + (p2-1)*z - p2;
                    zd = 3*z*z + p2-1;
                    z = z - p1*zf/zd,
                    0.001 <= |zf|
 }

RecipIke            { ; from REB001.FRM - by Ron Barnett 70153,1233
                    z = pixel:
                    z = 1/(z*z*z + (p1-1)*z - p1),
                    |z| <= 4
 }

F'FunctionIke       { ; generalized by Jon Horner
                      ; from RecipIke in REB001.FRM
                      : - by Ron Barnett 70153,1233
                    z = pixel:
                    z = fn1(z*z*z + (p1-1) * z - p1),
                    |z| <= 4
 }

IkeGenM             {; from REB002.FRM - by Ron Barnett 70153,1233
                    z = ((1-pixel)/(3*p1))^0.5:
                    z =p1*z*z*z + (pixel-1)*z - pixel,
                    |z| <= 100
 }

IkeGenJ             {; from REB002.FRM - by Ron Barnett 70153,1233
                    z = pixel:
                    z =p1*z*z*z + (p2-1)*z - p2,
                    |z| <= 100
 }
   
IkeFrRbtGenM        {; from REB002.FRM - by Ron Barnett 70153,1233
                    z = 2*(1-pixel)/(3*p1):
                    z = p1*z*z*z + (pixel-1)*z*z - pixel,
                    |z| <= 100
 }

IkeFrRbtGenJ        {; from REB002.FRM - by Ron Barnett 70153,1233
                    z = pixel:
                    z = p1*z*z*z + (p2-1)*z*z - p2,
                    |z| <= 100
 }

