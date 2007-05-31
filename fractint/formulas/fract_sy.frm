comment { 
  Because of its large size, this formula requires Fractint version 19.3 or
later to run.
  It uses Newton's formula applied to the equation z^6-1 = 0 and, in the 
foreground, spells out the word 'FRACTINT'.
}  

fractint [float=y periodicity=0]  {; Sylvie Gallet [101324,3444], 1996
            ; requires 'periodicity=0' 
 z = pixel-0.025 , x=real(z) , y=imag(z) , x1=x*1.8 , x3=3*x
 ty2 = ( (y<0.025) && (y>-0.025) ) || (y>0.175)
 f = ( (x<-1.2) || ty2 ) && ( (x>-1.25) && (x<-1) )
 r = ( (x<-0.9) || ty2 ) && ( (x>-0.95) && (x<-0.8) )
 r = r || ((cabs(sqrt(|z+(0.8,-0.1)|)-0.1)<0.025) && (x>-0.8))
 r = r || (((y<(-x1-1.44)) && (y>(-x1-1.53))) && (y<0.025))
 a = (y>(x3+1.5)) || (y>(-x3-1.2)) || ((y>-0.125) && (y<-0.075))
 a = a && ((y<(x3+1.65)) && (y<(-x3-1.05)))
 c = (cabs(sqrt(|z+0.05|)-0.2)<0.025) && (x<0.05)
 t1 = ((x>0.225) && (x<0.275) || (y>0.175)) && ((x>0.1) && (x<0.4))
 i = (x>0.45) && (x<0.5)
 n = (x<0.6) || (x>0.8) || ((y>-x1+1.215) && (y<-x1+1.305))
 n = n && (x>0.55) && (x<0.85)
 t2 = ((x>1.025) && (x<1.075) || (y>0.175)) && ((x>0.9) && (x<1.2))
 test = 1 - (real(f||r||a||c||t1||i||n||t2)*real(y>-0.225)*real(y<0.225)) 
 z = 1+(0.0,-0.65)/(pixel+(0.0,.75)) :
 z2 = z*z , z4 = z2*z2 , n = z4*z2-1 , z = z-n/(6*z4*z)
 (|n|>=0.0001) && test
}

comment {
  This formula uses Newton's formula applied to the real equation :
     F(x,y) = 0 where F(x,y) = (x^3 + y^2 - 1 , y^3 - x^2 + 1)
     starting with (x_0,y_0) = z0 = pixel
  It calculates:
     (x_(n+1),y_(n+1)) = (x_n,y_n) - (F'(x_n,y_n))^-1 * F(x_n,y_n)
     where (F'(x_n,y_n))^-1 is the inverse of the Jacobian matrix of F.
}
 
Newton_real [float=y]  { ; Sylvie Gallet [101324,3444], 1996
      ; Newton's method applied to   x^3 + y^2 - 1 = 0 
      ;                              y^3 - x^2 + 1 = 0
      ;                              solution (0,-1)
      ; One parameter : real(p1) = bailout value 
z = pixel , x = real(z) , y = imag(z) : 
 xy = x*y                                
 d = 9*xy+4 , x2 = x*x , y2 = y*y        
 c = 6*xy+2 
 x1 = x*c - (y*y2 - 3*y - 2)/x
 y1 = y*c + (x*x2 + 2 - 3*x)/y
 z = (x1+flip(y1))/d , x = real(z) , y = imag(z)
 (|x| >= p1) || (|y+1| >= p1)
}
