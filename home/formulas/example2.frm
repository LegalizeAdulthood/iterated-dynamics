{--- Bradley Beacham -----------------------------------------------------}

OK-32 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = y = x = pixel , k = 1 + p1 , test = 5 + p2 :
   a = fn1(z)
   if (a <= y)
      b = y
   else
      b = x
   endif
   x = y , y = z , z = a*k + b
   |z| <= test
}

OK-34 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = pixel , c = fn1(pixel) * p1 , test = 10 + p2 :
   x = abs(real(z)) , y = abs(imag(z))
   if (x <= y)
      z = fn2(z) + y + c
   else
      z = fn2(z) + x + c
   endif
   |z| <= test
}

OK-35 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = pixel, k = 1 + p1 , test = 10 + p2 :
   v = fn1(z) , x = z*v , y = z/v
   if (|x| <= |y|)
      z = fn2((z + y) * k * v) + v
   else
      z = fn2((z + x) * k * v) + v
   endif
   |z| <= test
}

Larry { ; Mutation of 'Michaelbrot' and 'Element'
   ; Original formulas by Michael Theroux
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Michaelbrot', set FN1 & FN2 =IDENT and P1 & P2 = default
   ; For 'Element', set FN1=IDENT & FN2=SQR and P1 & P2 = default
   ; p1 = Parameter (default 0.5,0), real(p2) = Bailout (default 4)
   z = pixel
   ; The next line sets c=default if p1=0, else c=p1
   if (real(p1) || imag(p1))
      c = p1
   else
      c = 0.5
   endif
   ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
   if (real(p2) <= 0)
      test = 4
   else
      test = real(p2)
   endif
   :
   z = fn1(fn2(z*z)) + c
   |z| <= test
}

Moe { ; Mutation of 'Zexpe'.
   ; Original formula by Lee Skinner
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Zexpe', set FN1 & FN2 = IDENT and P1 = default
   ; real(p1) = Bailout (default 100)
   s = exp(1.,0.), z = pixel, c = fn1(pixel)
   ; The next line sets test=100 if real(p1)<=0, else test=real(p1)
   if (real(p1) <= 0)
      test = 100
   else
      test = real(p1)
   endif
   :
   z = fn2(z)^s + c
   |z| <= test
}

Zeppo { ; Mutation of 'Liar4'.
   ; Original formula by Chuck Ebbert
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Liar4' set FN1 & FN2 =IDENT and P1 & P2 = default
   ; p1 & p2 = Parameters (default 1,0 and 0,0)
   z = pixel
   ; The next line sets p=default if p1=0, else p=p1
   if (real(p1) || imag(p1))
      p = p1
   else
      p = 1
   endif
   :
   z = fn1(1-abs(imag(z)*p-real(z))) +          \
       flip(fn2(1-abs(1-real(z)-imag(z)))) - p2
   |z| <= 1
}

comment {
  In this formula, a running count of the iterations is kept. After a
  specified iteration number has been reached, the algorithm is changed.
}

shifter01 { ; After shift, switch from z*z to z*z*z
            ; Bradley Beacham
            ; Modified for if..else logic 3/18/97 by Sylvie Gallet
   ; P1 = shift value, P2 varies bailout value
   z = c = pixel , iter = 1 , shift = p1 , test = 4 + p2 :
   if (iter <= shift)
      z = z*z + c
   else
      z = z*z*z + c
   endif
   iter = iter + 1
   |z| < test
}

{--- Robert W. Carr ------------------------------------------------------}

comment {
  This formula Is based on Sylvie Gallet's Five-Mandels formula.
  Though it shouldn't produce a symmetrical image, a modification of pixel
  forces symmetry around the Y axis.
}

Carr2289 (YAXIS) { ; Modified Sylvie Gallet frm., 1996
                   ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
   pixel = -abs(real(pixel)) + flip(imag(pixel))
   c = pixel + pixel - flip(0.001/pixel) - conj(0.01/pixel)
   z = zorig = pixel - conj(asin(pixel+pixel+0.32))
   d1 = flip(-0.0005935/pixel) , d4 = 4 * d1 , d5 = d1 + d4
   bailout = 4 , iter = 0 :
   if (iter == p1)
      z = c = 1.5 * zorig + d5
   elseif (iter == imag(p1))
      z = c = 2.25 * zorig + d5
   elseif (iter == p2)
      z = c = 3.375 * zorig + d5
   elseif (iter == imag(p2))
      z = c = 5.0625 * zorig + d5
   else
      z = z + d4 , c = c + d4
   endif
   z = z*z + c
   iter = iter + 1
   abs(z) <= bailout
}

{--- Sylvie Gallet -------------------------------------------------------}

Fractint { ; Sylvie Gallet, 1996
           ; Modified for if..else logic 3/21/97 by Sylvie Gallet
           ; requires 'periodicity=0'
   ; It uses Newton's formula applied to the equation z^6-1 = 0 and, in the
   ; foreground, spells out the word 'FRACTINT'.
   z = pixel-0.025 , x = real(z) , y = imag(z) , text = 0
   if (y > -0.225 && y < 0.225)
      x1 = x*1.8 , x3 = 3*x
      ty2 = y < 0.025 && y > -0.025 || y > 0.175
      if ( x < -1.2 || ty2 && x > -1.25 && x < -1 )
         text = 1
      elseif ( x < -0.9 || ty2 && x > -0.95 && x < -0.8                  \
               || (cabs(sqrt(|z+(0.8,-0.1)|)-0.1) < 0.025 && x > -0.8)   \
               || (y < -x1-1.44 && y > -x1-1.53 && y < 0.025) )
         text = 1
      elseif ( y > x3+1.5 || y > -x3-1.2 || (y > -0.125 && y < -0.075)   \
               && y < x3+1.65 && y < -x3-1.05 )
         text = 1
      elseif ( cabs(sqrt(|z+0.05|)-0.2) < 0.025 && x < 0.05 )
         text = 1
      elseif ( (x > 0.225 && x < 0.275 || y > 0.175) && x > 0.1 && x < 0.4 )
         text = 1
      elseif ( x > 0.45 && x < 0.5 )
         text = 1
      elseif ( x < 0.6 || x > 0.8 || ((y > -x1+1.215) && (y < -x1+1.305))  \
               && x > 0.55 && x < 0.85 )
         text = 1
      elseif ( x > 1.025 && x < 1.075 || y > 0.175 && x > 0.9 && x < 1.2 )
         text = 1
      endif
   endif
   z = 1 + (0.0,-0.65) / (pixel+(0.0,.75))
   :
   if (text == 0)
      z2 = z*z , z4 = z2*z2 , n = z4*z2-1 , z = z-n/(6*z4*z)
      if (|n| >= 0.0001)
         continue = 1
      else
         continue = 0
      endif
   endif
   continue
}

comment {
  Five-Mandels shows five Mandelbrot sets that fit into each other.
  It uses the following algorithm:
    z=c=pixel
    FOR iter:=0 to l1-1
      if the orbit of z*z + c escapes THEN end
        else
          z:=z1
          FOR iter:=L1+1 to l2-1
            if the orbit of z*z + z1 escapes THEN end
              else
                z:=z2
                FOR iter:=L2+1 to l3-1
                  ...
  To work correctly, this formula requires the use of periodicity=0.
}

Five-Mandels (XAXIS) {; Sylvie Gallet, 1996
   ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
   ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   c = z = zorig = pixel
   bailout = 16 , iter = 0 :
   if (iter == p1)
      z = c = 1.5 * zorig
   elseif (iter == imag(p1))
      z = c = 2.25 * zorig
   elseif (iter == p2)
      z = c = 3.375 * zorig
   elseif (iter == imag(p2))
      z = c = 5.0625 * zorig
   endif
   z = z*z + c
   iter = iter + 1
   |z| <= bailout
}

comment {
  The following formula draws the graphs Of 4 real functions at a time.
}

Graph { ; Sylvie Gallet, 1996
        ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   ; 2 parameters: curves thickness = real(p1)
   ;                 axes thickness = imag(p1)
   ; choose for example real(p1) = 0.002 and imag(p1) = 0.001
   epsilon = abs(real(p1)) , axes = abs(imag(p1))
   z = 0 , x = round(real(pixel)/epsilon) * epsilon
   if ((|real(pixel)| <= axes) || (|imag(pixel)| <= axes))
      z = z + 1
   endif
   if (|x + flip(fn1(x))-pixel| <= epsilon)
      z = z + 2
   endif
   if (|x + flip(fn2(x))-pixel| <= epsilon)
      z = z + 4
   endif
   if (|x + flip(fn3(x))-pixel| <= epsilon)
      z = z + 8
   endif
   if (|x + flip(fn4(x))-pixel| <= epsilon)
      z = z + 16
   endif
   if (z == 0)
      z = z + 100
   endif
   :
   z = z - 1
   z > 0
}

comment {
  The following formula overlays a Mandel And a reverse-Mandel, using a
  checkerboard dithering invisible at very high resolutions.
  Since it uses the new predefined variable "whitesq", it's now resolution
  independent and the image can be interrupted, saved and restored.
  Panning an even number of pixels is now possible.
}

JD-SG-04-1 { ; Sylvie Gallet, 1996
   ; On an original idea by Jim Deutch
   ; Modified for if..else logic 3/21/97 by Sylvie Gallet
   ; use p1 and p2 to adjust the inverted Mandel
   ; 16-bit Pseudo-HiColor
   if (whitesq)
      z = c = pixel
   else
      z = c = p1 / (pixel+p2)
   endif
   :
   z = z*z + c
   |z| < 4
}

comment {
  These formula overlay 3 Or 4 fractals.
}

ptc+mjn { ; Sylvie Gallet, 1996
          ; Modified for if..else logic 3/19/97 by Sylvie Gallet
          ; 24-bit Pseudo-TrueColor
          ; Mandel: z^2 + c , Julia: z^2 + p1 , Newton: z^p2 - 1 = 0
   cr = real(scrnpix) + imag(scrnpix)
   r = cr - 3 * trunc(cr / real(3)) , z = pixel
   if (r == 0)
      c = pixel , b1 = 256
   elseif (r == 1)
      c = p1 , b1 = 256
   else
      c = 0 , b2 = 0.000001 , ex = p2 - 1
   endif
   :
   if (r == 2)
      zd = z^ex , n = zd*z - 1
      z = z - n / (p2*zd) , continue = (|n| >= b2)
   else
      z = z*z + c , continue = (|z| <= b1)
   endif
   continue
}

ptc+4mandels { ; Sylvie Gallet, 1996
               ; 32-bit Pseudo-TrueColor
               ; Modified for if..else logic 3/21/97 by Sylvie Gallet
   cr = real(scrnpix) + 2*imag(scrnpix)
   r = cr - 4 * trunc(cr / 4)
   if (r == 0)
      z = c = pixel
   elseif (r == 1)
      z = c = pixel * p1
   elseif (r == 2)
      z = c = pixel * p2
   else
      z = c = pixel * p3
   endif
   :
   z = z * z + c
   |z| <= 4
}

Gallet-8 - 21 { ; Sylvie Gallet, Apr 1997
              ; Requires periodicity = 0 and decomp = 256
              ; p1 = parameter for a Julia set (0 for the Mandelbrot set)
              ; 0 < real(p2) , 0 < imag(p2)
   im2 = imag(p2)
   if (p1 || imag(p1))
      c = p1
   else
      c = pixel
   endif
   z = -1 , zn = pixel , zmin = zmin0 = abs(real(p2))
   cmax = trunc(abs(real(p3)))
   if (cmax < 2)
      cmax = 2
   endif
   k = flip(6.28318530718/(zmin*real(cmax))) , cnt = -1
   :
   cnt = cnt + 1
   if (cnt == cmax)
      cnt = 0
   endif
   zn = zn*zn + c , znc = cabs(im2*real(zn) + flip(imag(zn)))
   if (znc < zmin)
      zmin = znc , z = exp((cnt*zmin0 + zmin)*k)
   endif
   znc <= 4
}

{--- Terren Suydam -------------------------------------------------------}

comment {
  These formulas are designed to create tilings based on the Mandel or Julia
  formulas that can be used as HTML page or Windows backgrounds.
  Zoom in on a favorite spot on Mandel or Julia. Write down the center and
  magnification for that particular view. If it's a Julia, write down
  the real & imag. parameter as well.
  The numbers you write down will be parameters to the fractal type
  TileMandel or TileJulia.
  - For both, paramter p1 is the center of the image you want to tile.
  - The real part of p2 is the magnification (the default is 1/3).
  - The imag. part is the number of tiles you want to be drawn (the default
    is 3).
  - For TileJulia, p3 is the Julia parameter.
     These formulas need 'periodicity=0'.
}

TileMandel { ; Terren Suydam, 1996
             ; modified by Sylvie Gallet
             ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; p1 = center = coordinates for a good Mandel
   ; 0 <= real(p2) = magnification. Default for magnification is 1/3
   ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
   center = p1
   if (p2 > 0)
      mag = real(p2)
   else
      mag = 1/3
   endif
   if (imag(p2) > 0)
      numtiles = imag(p2)
   else
      numtiles = 3
   endif
   omega = numtiles*2*pi/3
   x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
   z = c = (x+flip(y)) / mag + center :
   z = z*z + c
   |z| <= 4
}

TileJulia { ; Terren Suydam, 1996
            ; modified by Sylvie Gallet
            ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; p1 = center = coordinates for a good Julia
   ; 0 <= real(p2) = magnification. Default for magnification is 1/3
   ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
   ; p3 is the Julia set parameter
   center = p1
   if (p2 > 0)
      mag = real(p2)
   else
      mag = 1/3
   endif
   if (imag(p2) > 0)
      numtiles = imag(p2)
   else
      numtiles = 3
   endif
   omega = numtiles*2*pi/3
   x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
   z = (x+flip(y)) / mag + center :
   z = z*z + p3
   |z| <= 4
}
