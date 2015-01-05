
Gallet-8-01 { ; Sylvie Gallet [101324,3444], Mar 1997
   z = c = pixel , zc = 0 :
   IF (zc < 0)
      z = z - p1
   ELSE
      z = z - zc - p1
   ENDIF
   zc = z*c
   |z| <= p2
   }

Gallet-8-03 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = c = zn = pixel :
   zn = zn*zn + c
   IF (|zn| < |z|)
      z = 0.6*zn
   ENDIF
   |zn| <= 4
   }

Gallet-8-04 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = zn = pixel , ex = p1 - 1
   IF (p2 || imag(p2))
      k = p2
   ELSE
      k = 1
   ENDIF
   :
   znex = zn^ex , num = znex*zn - 1 , den = p1*znex , zn = zn - num/den
   IF (|num| > |z^p1-1|)
      z = zn * k
   ENDIF
   |num| >= 0.001
   }

Gallet-8-05 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = c = zn = pixel
   IF (p1 || imag(p1))
      k = p1
   ELSE
      k = 1
   ENDIF
   :
   zn = zn*zn + c
   IF (abs(zn) < abs(z) || flip(abs(zn)) < flip(abs(z)))
      z = k*zn
   ENDIF
   |zn| <= 4
   }

Gallet-8-06 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = c = zn = pixel
   IF (p1 || imag(p1))
      k = p1
   ELSE
      k = 1
   ENDIF
   :
   zn = zn*zn + c
   IF (abs(zn) < abs(z) && flip(abs(zn)) < flip(abs(z)))
      z = k*zn
   ENDIF
   |zn| <= 4
   }

Gallet-8-07 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = c = zn = pixel
   IF (p1 || imag(p1))
      k = p1
   ELSE
      k = 1
   ENDIF
   :
   zn = zn*zn + c
   IF (abs(zn) < abs(z))
      z = k*real(zn) + flip(imag(z))
   ENDIF
   IF (flip(abs(zn)) < flip(abs(z)))
      z = real(z) + k*flip(imag(zn))
   ENDIF
   |zn| <= 4
   }

Gallet-8-08 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = zn = pixel
   IF (p2 || imag(p2))
      k = p2
   ELSE
      k = 1
   ENDIF
   :
   zn = zn*zn + p1
   IF (abs(zn) < abs(z) && flip(abs(zn)) < flip(abs(z)))
      z = k*zn
   ENDIF
   |zn| <= 4
   }

Gallet-8-11 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; PHC, requires periodicity = 0 and passes=1
   h = cabs(pixel) , r = real(p3) , ir = imag(p3)
   IF (h >= r)
      IF (whitesq)
         z = pixel , c = p1  ;trunc(p2*pixel)/p2
      ELSE
         z = 200
      ENDIF
   ELSE
      beta = asin(h/r) , alpha = asin(h/r/ir)
      h2 = h - sqrt(r*r - h*h) * tan(beta - alpha)
      z = h2*pixel/h, c = p1
   ENDIF
   :
   z = z*z + c
   |z| <= 128
   }

Gallet-8-12 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   h = cabs(pixel) , pinv = 1/p1
   bailout = 2*p1 , r = real(p2) , ir = imag(p2)
   IF (h >= r)
      z = pixel
   ELSE
      beta = asin(h/r) , alpha = asin(h/(r*ir))
      z = (h - sqrt(r*r - h*h) * tan(beta - alpha)) * pixel / h
   ENDIF
   center = round(p1*z) * pinv
   IF (cabs(z-center) < 0.45*pinv)
      z = cabs(center)
   ELSE
      z = cabs(center) + p1
   ENDIF
   :
   z = z + pinv
   z <= bailout
   }

Gallet-8-14 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = 0 , c = zn = pixel , zmin = p1 , k = flip(2*pi/zmin) :
   zn = zn*zn + c , znc = cabs(zn)
   IF (znc < zmin)
      zmin = znc , z = exp(zmin*k)
   ENDIF
   znc <= 4
   }

Gallet-8-15 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = 0 , zn = x = y = pixel , zmin = imag(p2) , k = flip(2*pi/zmin) :
   zn = zn*zn - 0.5*zn + p1 , x = zn*zn - 0.5*y + p1
   y = zn , zn = x , znc = cabs(zn)
   IF (znc < zmin)
      zmin = znc , z = exp(zmin*k)
   ENDIF
   znc <= real(p2)
   }

Gallet-8-16 { ; Sylvie Gallet [101324,3444], Mar 1997
              ; Requires periodicity = 0
   z = -1 , c = zn = pixel , xmin = ymin = p1
   odd = 0 , k = flip(pi/xmin) :
   zn = zn*zn + c , odd = odd==0
   IF (odd)
      IF (abs(zn) < xmin)
         xmin = abs(zn) , z = exp(xmin*k)
      ENDIF
   ELSE
      IF (abs(imag(zn)) < ymin)
         ymin = abs(imag(zn)) , z = exp(-ymin*k)
      ENDIF
   ENDIF
   |zn| <= 16
   }
