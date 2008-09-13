{--- BRADLEY BEACHAM -----------------------------------------------------}

OK-32 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = y = x = pixel , k = 1 + p1 , test = 5 + p2 :
   a = fn1(z)
   IF (a <= y)
      b = y
   ELSE
      b = x
   ENDIF
   x = y , y = z , z = a*k + b
   |z| <= test
   }

OK-34 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = pixel , c = fn1(pixel) * p1 , test = 10 + p2 :
   x = abs(real(z)) , y = abs(imag(z))
   IF (x <= y)
      z = fn2(z) + y + c
   ELSE
      z = fn2(z) + x + c
   ENDIF
   |z| <= test
   }

OK-35 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   z = pixel, k = 1 + p1 , test = 10 + p2 :
   v = fn1(z) , x = z*v , y = z/v
   IF (|x| <= |y|)
      z = fn2((z + y) * k * v) + v
   ELSE
      z = fn2((z + x) * k * v) + v
   ENDIF
   |z| <= test
   }

Larry { ; Mutation of 'Michaelbrot' and 'Element'
   ; Original formulas by Michael Theroux [71673,2767]
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Michaelbrot', set FN1 & FN2 =IDENT and P1 & P2 = default
   ; For 'Element', set FN1=IDENT & FN2=SQR and P1 & P2 = default
   ; p1 = Parameter (default 0.5,0), real(p2) = Bailout (default 4)
   z = pixel
   ; The next line sets c=default if p1=0, else c=p1
   IF (real(p1) || imag(p1))
      c = p1
   ELSE
      c = 0.5
   ENDIF
   ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
   IF (real(p2) <= 0)
      test = 4
   ELSE
      test = real(p2)
   ENDIF
   :
   z = fn1(fn2(z*z)) + c
   |z| <= test
   }

Moe { ; Mutation of 'Zexpe'.
   ; Original formula by Lee Skinner [75450,3631]
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Zexpe', set FN1 & FN2 = IDENT and P1 = default
   ; real(p1) = Bailout (default 100)
   s = exp(1.,0.), z = pixel, c = fn1(pixel)
   ; The next line sets test=100 if real(p1)<=0, else test=real(p1)
   IF (real(p1) <= 0)
      test = 100
   ELSE
      test = real(p1)
   ENDIF
   :
   z = fn2(z)^s + c
   |z| <= test
   }

Groucho { ; Mutation of 'Fish2'.
   ; Original formula by Dave Oliver via Tim Wegner
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Fish2', set FN1 & FN2 =IDENT and P1 & P2 = default
   ; p1 = Parameter (default 1,0), real(p2) = Bailout (default 4)
   z = c = pixel
   ; The next line sets k=default if p1=0, else k=p1
   IF (real(p1) || imag(p1))
      k = p1
   ELSE
      k = 1
   ENDIF
   ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
   IF (real(p2) <= 0)
      test = 4
   ELSE
      test = real(p2)
   ENDIF
   :
   z1 = c^(fn1(z)-k)
   z = fn2(((c*z1)-k)*(z1))
   |z| <= test
   }

Zeppo { ; Mutation of 'Liar4'.
   ; Original formula by Chuck Ebbert [76306,1226]
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Liar4' set FN1 & FN2 =IDENT and P1 & P2 = default
   ; p1 & p2 = Parameters (default 1,0 and 0,0)
   z = pixel
   ; The next line sets p=default if p1=0, else p=p1
   IF (real(p1) || imag(p1))
      p = p1
   ELSE
      p = 1
   ENDIF
   :
   z = fn1(1-abs(imag(z)*p-real(z))) +          \
       flip(fn2(1-abs(1-real(z)-imag(z)))) - p2
   |z| <= 1
   }

inandout02 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
  ;p1 = Parameter (default 0), real(p2) = Bailout (default 4)
  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
   IF (p2 <= 0)
      test = 4
   ELSE
      test = real(p2)
   ENDIF
   z = oldz = pixel , moldz = mz = |z| :
   IF (mz <= moldz)
      oldz = z , moldz = mz , z = fn1(z) + p1 , mz = |z|  ;IN
   ELSE
      oldz = z , moldz = mz , z = fn2(z) + p1 , mz = |z|  ;OUT
   ENDIF
   mz <= test
   }

inandout03 { ; Modified for if..else logic 3/19/97 by Sylvie Gallet
  ;p1 = Parameter (default 0), real(p2) = Bailout (default 4)
  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
   IF (p2 <= 0)
      test = 4
   ELSE
      test = real(p2)
   ENDIF
   z = oldz = c = pixel , moldz = mz = |z| :
   IF (mz <= moldz)
      c = fn1(c)       ;IN
   ELSE
      c = fn1(z * p1)  ;OUT
   ENDIF
   oldz = z , moldz = mz
   z = fn2(z*z) + c , mz = |z|
   mz <= test
   }

inandout04 { ; Modified for if..else logic 3/21/97 by Sylvie Gallet
  ;p1 = Parameter (default 1), real(p2) = Bailout (default 4)
   ; The next line sets k=default if p1=0, else k=p1
   IF (real(p1) || imag(p1))
      k = p1
   ELSE
      k = 1
   ENDIF
   ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
   IF (real(p2) <= 0)
      test = 4
   ELSE
      test = real(p2)
   ENDIF
   z = oldz = c = pixel , mz = moldz = |z|
   :
   IF (mz > moldz)
      c = c*k
   ENDIF
   oldz = z , moldz = mz , z = fn1(z*z) + c , mz = |z|
   mz <= test
   }

shifter01 { ; After shift, switch from z*z to z*z*z
            ; Bradley Beacham  [74223,2745]
            ; Modified for if..else logic 3/18/97 by Sylvie Gallet
   ; P1 = shift value, P2 varies bailout value
   z = c = pixel , iter = 1 , shift = p1 , test = 4 + p2 :
   IF (iter <= shift)
      z = z*z + c
   ELSE
      z = z*z*z + c
   ENDIF
   iter = iter + 1
   |z| < test
   }

{--- ROBERT W. CARR ------------------------------------------------------}

Carr2289 (YAXIS) { ; Modified Sylvie Gallet frm. [101324,3444],1996
                   ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
   pixel = -abs(real(pixel)) + flip(imag(pixel))
   c = pixel + pixel - flip(0.001/pixel) - conj(0.01/pixel)
   z = zorig = pixel - conj(asin(pixel+pixel+0.32))
   d1 = flip(-0.0005935/pixel) , d4 = 4 * d1 , d5 = d1 + d4
   bailout = 4 , iter = 0 :
   IF (iter == p1)
      z = c = 1.5 * zorig + d5
   ELSEIF (iter == imag(p1))
      z = c = 2.25 * zorig + d5
   ELSEIF (iter == p2)
      z = c = 3.375 * zorig + d5
   ELSEIF (iter == imag(p2))
      z = c = 5.0625 * zorig + d5
   ELSE
      z = z + d4 , c = c + d4
   ENDIF
   z = z*z + c
   iter = iter + 1
   abs(z) <= bailout
   }

{--- SYLVIE GALLET -------------------------------------------------------}

Fractint {; Sylvie Gallet [101324,3444], 1996
          ; Modified for if..else logic 3/21/97 by Sylvie Gallet
          ; requires 'periodicity=0'
   z = pixel-0.025 , x = real(z) , y = imag(z) , text = 0
   IF (y > -0.225 && y < 0.225)
      x1 = x*1.8 , x3 = 3*x
      ty2 = y < 0.025 && y > -0.025 || y > 0.175
      IF ( x < -1.2 || ty2 && x > -1.25 && x < -1 )
         text = 1
      ELSEIF ( x < -0.9 || ty2 && x > -0.95 && x < -0.8                  \
               || (cabs(sqrt(|z+(0.8,-0.1)|)-0.1) < 0.025 && x > -0.8)   \
               || (y < -x1-1.44 && y > -x1-1.53 && y < 0.025) )
         text = 1
      ELSEIF ( y > x3+1.5 || y > -x3-1.2 || (y > -0.125 && y < -0.075)   \
               && y < x3+1.65 && y < -x3-1.05 )
         text = 1
      ELSEIF ( cabs(sqrt(|z+0.05|)-0.2) < 0.025 && x < 0.05 )
         text = 1
      ELSEIF ( (x > 0.225 && x < 0.275 || y > 0.175) && x > 0.1 && x < 0.4 )
         text = 1
      ELSEIF ( x > 0.45 && x < 0.5 )
         text = 1
      ELSEIF ( x < 0.6 || x > 0.8 || ((y > -x1+1.215) && (y < -x1+1.305))  \
               && x > 0.55 && x < 0.85 )
         text = 1
      ELSEIF ( x > 1.025 && x < 1.075 || y > 0.175 && x > 0.9 && x < 1.2 )
         text = 1
      ENDIF
   ENDIF
   z = 1 + (0.0,-0.65) / (pixel+(0.0,.75))
   :
   IF (text == 0)
      z2 = z*z , z4 = z2*z2 , n = z4*z2-1 , z = z-n/(6*z4*z)
      IF (|n| >= 0.0001)
         continue = 1
      ELSE
         continue = 0
      ENDIF
   ENDIF
   continue
   }

Five-Mandels (XAXIS) {; Sylvie Gallet [101324,3444], 1996
   ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
   ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   c = z = zorig = pixel
   bailout = 16 , iter = 0 :
   IF (iter == p1)
      z = c = 1.5 * zorig
   ELSEIF (iter == imag(p1))
      z = c = 2.25 * zorig
   ELSEIF (iter == p2)
      z = c = 3.375 * zorig
   ELSEIF (iter == imag(p2))
      z = c = 5.0625 * zorig
   ENDIF
   z = z*z + c
   iter = iter + 1
   |z| <= bailout
   }

Graph { ; Sylvie Gallet [101324,3444], 1996
        ; Modified for if..else logic 3/17/97 by Sylvie Gallet
   ; 2 parameters: curves thickness = real(p1)
   ;                 axes thickness = imag(p1)
   ; choose for example real(p1) = 0.002 and imag(p1) = 0.001
   epsilon = abs(real(p1)) , axes = abs(imag(p1))
   z = 0 , x = round(real(pixel)/epsilon) * epsilon
   IF ((|real(pixel)| <= axes) || (|imag(pixel)| <= axes))
      z = z + 1
   ENDIF
   IF (|x + flip(fn1(x))-pixel| <= epsilon)
      z = z + 2
   ENDIF
   IF (|x + flip(fn2(x))-pixel| <= epsilon)
      z = z + 4
   ENDIF
   IF (|x + flip(fn3(x))-pixel| <= epsilon)
      z = z + 8
   ENDIF
   IF (|x + flip(fn4(x))-pixel| <= epsilon)
      z = z + 16
   ENDIF
   IF (z == 0)
      z = z + 100
   ENDIF
   :
   z = z - 1
   z > 0
   }

JD-SG-04-1 { ; Sylvie Gallet [101324,3444], 1996
   ; On an original idea by Jim Deutch [104074,3171]
   ; Modified for if..else logic 3/21/97 by Sylvie Gallet
   ; use p1 and p2 to adjust the inverted Mandel
   ; 16-bit Pseudo-HiColor
   IF (whitesq)
      z = c = pixel
   ELSE
      z = c = p1 / (pixel+p2)
   ENDIF
   :
   z = z*z + c
   |z| < 4
   }

ptc+mjn { ; Sylvie Gallet [101324,3444], 1996
          ; Modified for if..else logic 3/19/97 by Sylvie Gallet
          ; 24-bit Pseudo-TrueColor
          ; Mandel: z^2 + c , Julia: z^2 + p1 , Newton: z^p2 - 1 = 0
   cr = real(scrnpix) + imag(scrnpix)
   r = cr - 3 * trunc(cr / real(3)) , z = pixel
   IF (r == 0)
      c = pixel , b1 = 256
   ELSEIF (r == 1)
      c = p1 , b1 = 256
   ELSE
      c = 0 , b2 = 0.000001 , ex = p2 - 1
   ENDIF
   :
   IF (r == 2)
      zd = z^ex , n = zd*z - 1
      z = z - n / (p2*zd) , continue = (|n| >= b2)
   ELSE
      z = z*z + c , continue = (|z| <= b1)
   ENDIF
   continue
   }

ptc+4mandels { ; Sylvie Gallet [101324,3444], 1996
               ; 32-bit Pseudo-TrueColor
               ; Modified for if..else logic 3/21/97 by Sylvie Gallet
   cr = real(scrnpix) + 2*imag(scrnpix)
   r = cr - 4 * trunc(cr / 4)
   IF (r == 0)
      z = c = pixel
   ELSEIF (r == 1)
      z = c = pixel * p1
   ELSEIF (r == 2)
      z = c = pixel * p2
   ELSE
      z = c = pixel * p3
   ENDIF
   :
   z = z * z + c
   |z| <= 4
   }

{--- JONATHAN OSUCH ------------------------------------------------------}

BirdOfPrey (XAXIS_NOPARM) { ; Optimized by Sylvie Gallet
  z = p1 :
   z = cosxx(sqr(z) + pixel) + pixel
    |z| <= 4
  }

FractalFenderC (XAXIS_NOPARM) { ; Spectacular!
   ; Modified for if..else logic 3/18/97 by Sylvie Gallet
   z = p1 , x = |z| :
   IF (1 < x)
      z = cosh(z) + pixel
   ENDIF
   z = sqr(z) + pixel , x = |z|
   x <= 4
   }

{--- TERREN SUYDAM -------------------------------------------------------}

TileMandel { ; Terren Suydam (terren@io.com), 1996
             ; modified by Sylvie Gallet [101324,3444]
             ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; p1 = center = coordinates for a good Mandel
   ; 0 <= real(p2) = magnification. Default for magnification is 1/3
   ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
   center = p1
   IF (p2 > 0)
      mag = real(p2)
   ELSE
      mag = 1/3
   ENDIF
   IF (imag(p2) > 0)
      numtiles = imag(p2)
   ELSE
      numtiles = 3
   ENDIF
   omega = numtiles*2*pi/3
   x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
   z = c = (x+flip(y)) / mag + center :
   z = z*z + c
   |z| <= 4
   }

TileJulia { ; Terren Suydam (terren@io.com), 1996
            ; modified by Sylvie Gallet [101324,3444]
            ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; p1 = center = coordinates for a good Julia
   ; 0 <= real(p2) = magnification. Default for magnification is 1/3
   ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
   ; p3 is the Julia set parameter
   center = p1
   IF (p2 > 0)
      mag = real(p2)
   ELSE
      mag = 1/3
   ENDIF
   IF (imag(p2) > 0)
      numtiles = imag(p2)
   ELSE
      numtiles = 3
   ENDIF
   omega = numtiles*2*pi/3
   x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
   z = (x+flip(y)) / mag + center :
   z = z*z + p3
   |z| <= 4
   }
