comment {

  This formula file, released with Fractint 19.6, contains the new versions
  of all the Fractint.frm formulas that use conditional statements, followed
  by the original version commented out.

}

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

; Original version
; OK-32 {
;  z = y = x = pixel, k = 1 + p1:
;   a = fn1(z)
;   b = (a <= y) * ((a * k) + y)
;   e = (a > y) * ((a * k) + x)
;   x = y
;   y = z
;   z = b + e
;    |z| <= (5 + p2)
;  }

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

; Original version
; OK-34 {
;  z = pixel, c = (fn1(pixel) * p1):
;   x = abs(real(z))
;   y = abs(imag(z))
;   a = (x <= y) * (fn2(z) + y + c)
;   b = (x > y) * (fn2(z) + x + c)
;   z = a + b
;    |z| <= (10 + p2)
;  }

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

; Original version
; OK-35 {
;  z = pixel, k = 1 + p1:
;   v = fn1(z)
;   x = (z*v)
;   y = (z/v)
;   a = (|x| <= |y|) * ((z + y) * k)
;   b = (|x| > |y|) * ((z + x) * k)
;   z = fn2((a + b) * v) + v
;    |z| <= (10 + p2)
;  }

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

; Original version
; Larry { ; Mutation of 'Michaelbrot' and 'Element'
;  ; Original formulas by Michael Theroux [71673,2767]
;  ; For 'Michaelbrot', set FN1 & FN2 =IDENT and P1 & P2 = default
;  ; For 'Element', set FN1=IDENT & FN2=SQR and P1 & P2 = default
;  ; p1 = Parameter (default 0.5,0), real(p2) = Bailout (default 4)
;  z = pixel
;  ; The next line sets c=default if p1=0, else c=p1
;  c = ((0.5,0) * (|p1|<=0) + p1)
;  ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
;  test = (4 * (real(p2)<=0) + real(p2) * (0<p2)):
;   z = fn1(fn2(z*z)) + c
;    |z| <= test
;  }

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

; Original version
; Moe { ; Mutation of 'Zexpe'.
;  ; Original formula by Lee Skinner [75450,3631]
;  ; For 'Zexpe', set FN1 & FN2 =IDENT and P1 = default
;  ; real(p1) = Bailout (default 100)
;  s = exp(1.,0.), z = pixel, c = fn1(pixel)
;  ; The next line sets test=100 if real(p1)<=0, else test=real(p1)
;  test = (100 * (real(p1)<=0) + real(p1) * (0<p1)):
;   z = fn2(z)^s + c
;    |z| <= test
;  }

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

; Original version
; Groucho { ; Mutation of 'Fish2'.
;  ; Original formula by Dave Oliver via Tim Wegner
;  ; For 'Fish2', set FN1 & FN2 =IDENT and P1 & P2 = default
;  ; p1 = Parameter (default 1,0), real(p2) = Bailout (default 4)
;  z = c = pixel
;  ; The next line sets k=default if p1=0, else k=p1
;  k = ((1,0) * (|p1|<=0) + p1)
;  ; The next line sets test=4 if real(p2)<=0, else test=real(p2)
;  test = (4 * (real(p2)<=0) + real(p2) * (0<p2)):
;   z1 = c^(fn1(z)-k)
;   z = fn2(((c*z1)-k)*(z1))
;    |z| <= test
;  }

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

; Original version
; Zeppo { ; Mutation of 'Liar4'.
;  ; Original formula by Chuck Ebbert [76306,1226]
;  ; For 'Liar4' set FN1 & FN2 =IDENT and P1 & P2 = default
;  ; p1 & p2 = Parameters (default 1,0 and 0,0)
;  z = pixel
;  ; The next line sets p=default if p1=0, else p=p1
;  p = (1 * (|p1|<=0) + p1):
;   z =fn1(1-abs(imag(z)*p-real(z)))+flip(fn2(1-abs(1-real(z)-imag(z))))-p2
;    |z| <= 1
;  }

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

; Original version
; inandout02 {
;  ;p1 = Parameter (default 0), real(p2) = Bailout (default 4)
;  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
;  test = (4 * (real(p2)<=0) + real(p2) * (0<p2))
;  z = oldz = pixel:
;   a = (|z| <= |oldz|) * (fn1(z)) ;IN
;   b = (|oldz| < |z|) * (fn2(z))  ;OUT
;   oldz = z
;   z = a + b + p1
;    |z| <= test
;  }

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

; Original version
; inandout03 {
;  ;p1 = Parameter (default 0), real(p2) = Bailout (default 4)
;  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
;  test = (4 * (real(p2)<=0) + real(p2) * (0<p2))
;  z = oldz = c = pixel:
;   a = (|z| <= |oldz|) * (c)    ;IN
;   b = (|oldz| < |z|)  * (z*p1) ;OUT
;   c = fn1(a + b)
;   oldz = z
;   z = fn2(z*z) + c
;    |z| <= test
;  }

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

; Original version
; inandout04 {
;  ;p1 = Parameter (default 1), real(p2) = Bailout (default 4)
;  ;The next line sets k=default if p1=0, else k=p1
;  k = ((1) * (|p1|<=0) + p1)
;  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
;  test = (4 * (real(p2)<=0) + real(p2) * (0<p2))
;  z = oldz = c = pixel:
;   a = (|z| <= |oldz|) * (c)   ;IN
;   b = (|oldz| < |z|)  * (c*k) ;OUT
;   c = a + b
;   oldz = z
;   z = fn1(z*z) + c
;    |z| <= test
;  }

comment {
  In this formula, a running count of the iterations is kept. After a
  specified iteration number has been reached, the algorithm is changed.
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

; Original version
; shifter01 { ; After shift, switch from z*z to z*z*z
;            ; Bradley Beacham  [74223,2745]
;  ; P1 = shift value, P2 varies bailout value
;  z = c = pixel, iter = 1, shift = p1, test = 4 + p2:
;   lo = (z*z) * (iter <= shift)
;   hi = (z*z*z) * (shift < iter)
;   iter = iter + 1
;   z = lo + hi + c
;    |z| < test
;  }

{--- ROBERT W. CARR ------------------------------------------------------}

COMMENT {
  This formula is based on Sylvie Gallet's Five-Mandels formula.
  Though it shouldn't produce a symmetrical image, a modification of pixel
  forces symmetry around the Y axis.
  }

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

; Original version
; Carr2289 (YAXIS) {; Modified Sylvie Gallet frm. [101324,3444],1996
;  ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
;  pixel = -abs(real(pixel)) + flip(imag(pixel))
;  c = pixel + pixel - flip(0.001/pixel) - conj(0.01/pixel)
;  z = pixel - conj(asin(pixel+pixel+0.32))
;  d1 = flip(-0.0005935/pixel) , d4 = 4*d1
;  z1 = 1.5*z+d1 , z2 = 2.25*z+d1 , z3 = 3.375*z+d1 , z4 = 5.0625*z+d1
;  l1 = real(p1) , l2 = imag(p1) , l3 = real(p2) , l4 = imag(p2)
;  bailout = 16 , iter = 0 :
;   t1 = iter==l1 , t2 = iter==l2 , t3 = iter==l3 , t4 = iter==l4
;   t = 1 - (t1||t2||t3||t4) , ct = z1*t1 + z2*t2 + z3*t3 + z4*t4 + d4
;   z = z*t + ct , c = c*t + ct
;   z = z*z + c
;   iter = iter + 1
;    |real(z)| <= bailout
;  }

{--- SYLVIE GALLET -------------------------------------------------------}

comment {
  Because of its large size, this formula requires Fractint version 19.3 or
  later to run.
  It uses Newton's formula applied to the equation z^6-1 = 0 and, in the
  foreground, spells out the word 'FRACTINT'.
  }

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

; Original version
; Fractint {; Sylvie Gallet [101324,3444], 1996
;          ; requires 'periodicity=0'
;  z = pixel-0.025 , x=real(z) , y=imag(z) , x1=x*1.8 , x3=3*x
;  ty2 = ( (y<0.025) && (y>-0.025) ) || (y>0.175)
;  f = ( (x<-1.2) || ty2 ) && ( (x>-1.25) && (x<-1) )
;  r = ( (x<-0.9) || ty2 ) && ( (x>-0.95) && (x<-0.8) )
;  r = r || ((cabs(sqrt(|z+(0.8,-0.1)|)-0.1)<0.025) && (x>-0.8))
;  r = r || (((y<(-x1-1.44)) && (y>(-x1-1.53))) && (y<0.025))
;  a = (y>(x3+1.5)) || (y>(-x3-1.2)) || ((y>-0.125) && (y<-0.075))
;  a = a && ((y<(x3+1.65)) && (y<(-x3-1.05)))
;  c = (cabs(sqrt(|z+0.05|)-0.2)<0.025) && (x<0.05)
;  t1 = ((x>0.225) && (x<0.275) || (y>0.175)) && ((x>0.1) && (x<0.4))
;  i = (x>0.45) && (x<0.5)
;  n = (x<0.6) || (x>0.8) || ((y>-x1+1.215) && (y<-x1+1.305))
;  n = n && (x>0.55) && (x<0.85)
;  t2 = ((x>1.025) && (x<1.075) || (y>0.175)) && ((x>0.9) && (x<1.2))
;  test = 1 - (real(f||r||a||c||t1||i||n||t2)*real(y>-0.225)*real(y<0.225))
;  z = 1+(0.0,-0.65)/(pixel+(0.0,.75)) :
;   z2 = z*z , z4 = z2*z2 , n = z4*z2-1 , z = z-n/(6*z4*z)
;    (|n|>=0.0001) && test
;  }

comment {
  Five-Mandels shows five Mandelbrot sets that fit into each other.
  It uses the following algorithm:
    z=c=pixel
    FOR iter:=0 to l1-1
      IF the orbit of z*z + c escapes THEN end
        ELSE
          z:=z1
          FOR iter:=L1+1 to l2-1
            IF the orbit of z*z + z1 escapes THEN end
              ELSE
                z:=z2
                FOR iter:=L2+1 to l3-1
                  ...
  To work correctly, this formula requires the use of periodicity=0.
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

; Original version
; Five-Mandels (XAXIS) {; Sylvie Gallet [101324,3444], 1996
;  ; 0 < real(p1) < imag(p1) < real(p2) < imag(p2) < maxiter, periodicity=0
;  c = z = pixel
;  z1 = 1.5*z , z2 = 2.25*z , z3 = 3.375*z , z4 = 5.0625*z
;  l1 = real(p1) , l2 = imag(p1) , l3 = real(p2) , l4 = imag(p2)
;  bailout = 16 , iter = 0 :
;   t1 = (iter==l1) , t2 = (iter==l2) , t3 = (iter==l3) , t4 = (iter==l4)
;   t = 1-(t1||t2||t3||t4) , ct = z1*t1 + z2*t2 + z3*t3 + z4*t4
;   z = z*t + ct , c = c*t + ct
;   z = z*z + c
;   iter = iter+1
;    |z| <= bailout
;  }

comment {
  The following formula draws the graphs of 4 real functions at a time.
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

; Original version
; Graph { ; Sylvie Gallet [101324,3444], 1996
;  ; 2 parameters: curves thickness = real(p1)
;  ;                 axes thickness = imag(p1)
;  ; choose for example real(p1) = 0.002 and imag(p1) = 0.001
;  epsilon = abs(real(p1)) , axes = abs(imag(p1))
;  x = round(real(pixel)/epsilon) * epsilon
;  z1 = x + flip(fn1(x)) , z2 = x + flip(fn2(x))
;  z3 = x + flip(fn3(x)) , z4 = x + flip(fn4(x))
;  testaxes = (|real(pixel)|<=axes) || (|imag(pixel)|<=axes)
;  testfn1 = 2*(|z1-pixel|<=epsilon) , testfn2 = 4*(|z2-pixel|<=epsilon)
;  testfn3 = 8*(|z3-pixel|<=epsilon) , testfn4 = 16*(|z4-pixel|<=epsilon)
;  z = testaxes + testfn1 + testfn2 + testfn3 + testfn4
;  z = z + 100*(z==0) :
;   z = z - 1
;    z > 0
;  }

comment {
  The following formula overlays a Mandel and a reverse-Mandel, using a
  checkerboard dithering invisible at very high resolutions.
  Since it uses the new predefined variable "whitesq", it's now resolution
  independent and the image can be interrupted, saved and restored.
  Panning an even number of pixels is now possible.
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

; Original version
; JD-SG-04-1 { ; Sylvie Gallet [101324,3444], 1996
;  ; On an original idea by Jim Deutch [104074,3171]
;  ; use p1 and p2 to adjust the inverted Mandel
;  ; 16-bit Pseudo-HiColor
;  z = c = pixel * whitesq + (p1 / (pixel+p2)) * (whitesq==0) :
;   z = z*z + c
;    |z| < 4
;  }

comment {
  These formula overlay 3 or 4 fractals.
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

; Original version
; ptc+mjn { ; Sylvie Gallet [101324,3444], 1996
;          ; 24-bit Pseudo-TrueColor
;          ; Mandel: z^2 + c , Julia: z^2 + p1 , Newton: z^p2 - 1 = 0
;  cr = real(scrnpix) + imag(scrnpix)
;  r = cr - 3 * trunc(cr / real(3))
;  z = pixel , b1 = 256 , b2 = 0.000001 , ex = p2 - 1
;  c = pixel * (r==0) + p1 * (r==1) :
;   zd = z^ex , zn = zd*z , n = zn - 1 , d = p2 * zd
;   z = (z*z + c) * (r!=2) + (z - n/d) * (r==2)
;    ((|z| <= b1) && (r!=2)) || ((|n| >= b2 ) && (r==2))
;  }

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

; Original version
; ptc+4mandels { ; Sylvie Gallet [101324,3444], 1996
;               ; 32-bit Pseudo-TrueColor
;  cr = real(scrnpix) + 2*imag(scrnpix)
;  r = cr - 4 * trunc(cr / 4)
;  c = r == 0 , c1 = p1 * (r == 1)
;  c2 = p2 * (r == 2) , c3 = p3 * (r == 3)
;  z = c = pixel * (c + c1 + c2 + c3) :
;   z = z * z + c
;    |z| <= 4
;  }

Gallet-8-21 { ; Sylvie Gallet [101324,3444], Apr 1997
              ; Requires periodicity = 0 and decomp = 256
              ; p1 = parameter for a Julia set (0 for the Mandelbrot set)
              ; 0 < real(p2) , 0 < imag(p2)
   im2 = imag(p2)
   IF (p1 || imag(p1))
      c = p1
   ELSE
      c = pixel
   ENDIF
   z = -1 , zn = pixel , zmin = zmin0 = abs(real(p2))
   cmax = trunc(abs(real(p3)))
   IF (cmax < 2)
      cmax = 2
   ENDIF
   k = flip(6.28318530718/(zmin*real(cmax))) , cnt = -1
   :
   cnt = cnt + 1
   IF (cnt == cmax)
      cnt = 0
   ENDIF
   zn = zn*zn + c , znc = cabs(im2*real(zn) + flip(imag(zn)))
   IF (znc < zmin)
      zmin = znc , z = exp((cnt*zmin0 + zmin)*k)
   ENDIF
   znc <= 4
   }

{--- JONATHAN OSUCH ------------------------------------------------------}

BirdOfPrey (XAXIS_NOPARM) { ; Optimized by Sylvie Gallet
  z = p1 :
   z = cosxx(sqr(z) + pixel) + pixel
    |z| <= 4
  }

; Original version
; BirdOfPrey(XAXIS_NOPARM) {
;  z=p1, x=1:
;   (x<10)*(z=sqr(z)+pixel)
;   (10<=x)*(z=cosxx(z)+pixel)
;   x=x+1
;    |z|<=4
;  }

FractalFenderC (XAXIS_NOPARM) { ; Spectacular!
   ; Modified for if..else logic 3/18/97 by Sylvie Gallet
   z = p1 , x = |z| :
   IF (1 < x)
      z = cosh(z) + pixel
   ENDIF
   z = sqr(z) + pixel , x = |z|
   x <= 4
   }

; Original version
; FractalFenderC(XAXIS_NOPARM) {;Spectacular!
;  z=p1,x=|z|:
;   (z=cosh(z)+pixel)*(1<x)+(z=z)*(x<=1)
;   z=sqr(z)+pixel,x=|z|
;    x<=4
;  }

{--- TERREN SUYDAM -------------------------------------------------------}

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

; Original version
; TileMandel { ; Terren Suydam (terren@io.com), 1996
;             ; modified by Sylvie Gallet [101324,3444]
;  ; p1 = center = coordinates for a good Mandel
;  ; 0 <= real(p2) = magnification. Default for magnification is 1/3
;  ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
;  center = p1 , mag = real(p2)*(p2>0) + (p2<=0)/3
;  numtiles = imag(p2)*(flip(p2)>0) + 3*(flip(p2)<=0)
;  omega = numtiles*2*pi/3
;  x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
;  z = c = (x+flip(y)) / mag + center :
;   z = z*z + c
;    |z| <= 4
;  }

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

; Original version
; TileJulia { ; Terren Suydam (terren@io.com), 1996
;            ; modified by Sylvie Gallet [101324,3444]
;  ; p1 = center = coordinates for a good Julia
;  ; 0 <= real(p2) = magnification. Default for magnification is 1/3
;  ; 0 <= imag(p2) = numtiles. Default for numtiles is 3
;  ; p3 is the Julia set parameter
;  center = p1 , mag = real(p2)*(p2>0) + (p2<=0)/3
;  numtiles = imag(p2)*(flip(p2)>0) + 3*(flip(p2)<=0)
;  omega = numtiles*2*pi/3
;  x = asin(sin(omega*real(pixel))) , y = asin(sin(omega*imag(pixel)))
;  z = (x+flip(y)) / mag + center :
;   z = z*z + p3
;    |z| <= 4
;  }
