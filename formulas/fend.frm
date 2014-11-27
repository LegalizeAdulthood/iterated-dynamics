FractalFenderCS (XAXIS_NOPARM) { ; Spectacular!
   ; Modified for if..else logic 3/18/97 by Sylvie Gallet
   z = p1 , x = |z| :
   IF (x<=1)
      (z=cosh(z)+pixel)*(1<x)+(z=z)*(x<=1)
   else
      (z=cosh(z)+pixel)*(1<x)+(z=z)*(x<=1)
   ENDIF
   z = sqr(z) + pixel , x = |z|
   x <= 4
   }


