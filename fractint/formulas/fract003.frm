RCL_Pick1 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Try corners=2.008874/-3.811126/-3.980167/3.779833/
   ; -3.811126/3.779833 to see Figure 9.7 (P. 123) in
   ; Pickover's Computers, Pattern, Chaos and Beauty.
   ; Figures 9.9 - 9.13 can be found by zooming.
   ; Use floating point
   z=0:
   z=cosh(z) + pixel,
   abs(z) < 40
   }

RCL_Pick10 (XAXIS) { ; Ron Lewen, 76376,2567
   ;  Variation of Figure 9.18 (p.134) from Pickover's
   ;  Book.  Generates an interesting Biomorph.
   z=pixel:
   z=z/pixel-pixel*sqr(z),
   abs(z) < 8
   }

RCL_Pick11 (XAXIS) { ; Ron Lewen, 76376,2567
   ;  Formula from Figure 8.3 (p. 98) of Pickover's
   ;  book.  Generates a biomorph.  Figure 8.3 is a
   ;  zoom on one of the shapes at the corner of the
   ;  biomorph.
   ;  Use Floating Point
   z=pixel:
   z=z^2+0.5
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Pick12 { ; Ron Lewen, 76376,2567
   ;  Formula from Figure 12.7 (p. 202) of Pickover's
   ;  book.
   ;  Use Floating Point
   z=pixel:
   z=(2.71828^(p1)) * z * (1-z),
   abs(real(z)) < 10 || abs(imag(z)) < 10
   }

RCL_Pick13 { ; Ron Lewen, 76376,2567
   ;  Formula from Frontpiece for Appendix C
   ;  and Credits in Pickover's book.
   ;  Set p1=(3,0) to generate the Frontpiece
   ;  for Appendix C and to (2,0) for Credits
   ;  Use Floating Point
   z=.001:
   z=z^p1+(1/pixel)^p1,
   |z| <= 100
   }

RCL_Pick2_J { ; Ron Lewen, 76376,2567
   ;  A julia set based on the formula in Figure 8.9
   ;  (p. 105) of Pickover's book.  Very similar to
   ;  the Frontpiece for Appendix A.
   z=pixel:
   z=sin(z) + z^2 + p1,
   abs(real(z)) < 100 || abs(imag(z)) < 100
   }

RCL_Pick2_M (XAXIS) { ; Ron Lewen, 76376,2567
   ; Generates a biomorph of a Pseudo-Mandelbrot set with
   ; extra tails.  Part of Pickover's Biomorph Zoo Collection
   ; Formula is adapted from Pickover's book, Figure 8.9
   ; (p. 105) but the result is different.  Set corners=
   ; -2.640801/1.359199/-1.5/1.5 to center image.  I use the
   ; color map that comes as default in WINFRACT. (I guess I
   ; like purple <G>).
   ; Use floating point
   z=pixel:
   z=sin(z) + z^2 + pixel,
   |real(z)| < 100 || |imag(z)| < 100
   }

RCL_Pick3 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Generates Figure 9.18 (p. 134) from Pickover's book.
   ; Set maxiter >= 1000 to see good detail in the spirals
   ; in the three large lakes.  Also set inside=0.
   z=0.5:
   z=z*pixel-pixel/sqr(z),
   abs(z) < 8
   }

RCL_Pick4 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Variation of formula for Figure 9.18 (p. 134) from Pickover's
   ; book.
   ; Set inside=0 to see three large lakes around a blue "core".
   z=pixel:
   z=z*pixel-pixel/sqr(z),
   |z| <= 4
   }

RCL_Pick5 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Adapted from Pickover's Biomorph Zoo Collection in
   ; Figure 8.7 (p. 102).
   z=pixel:
   z=z^z + z^5 + pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Pick6 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Adapted from Pickover's Biomorph Zoo Collection in
   ; Figure 8.7 (p. 102).
   z=pixel:
   z=z^z + z^6 + pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Pick7 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Adapted from Pickover's Biomorph Zoo Collection in
   ; Figure 8.7 (p. 102).
   z=pixel:
   z=z^5 + pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Pick8 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Adapted from Pickover's Biomorph Zoo Collection in
   ; Figure 8.7 (p. 102).
   z=pixel:
   z=z^3 + pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Pick9 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Adapted from Pickover's Biomorph Zoo Collection in
   ; Figure 8.7 (p. 102).
   z=pixel:
   z=sin(z) + 2.71828^z + pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_Quaternion_J (ORIGIN) { ; Ron Lewen, 76376,2567
   ;  From Pseudocode 10.56 (p. 169) of Pickover's book.
   ;  Looks at Julia set for a0,a2 plane.  p1 selects
   ;  slice in to look at.
   ;  p2 corresponds to a point on the Quaternion
   ;  Mandelbrot set (see below).
   ;  Try (-.745,.113) as a starting point.
   a0=real(pixel), a2=imag(pixel), a1=real(p1), a3=imag(p1):
   savea0=a0^2-a1^2-a2^2-a3^2+p2,
   savea2=2*a0*a2+p2, a0=savea0, a2=savea2,
   (a0^2+a1^2+a2^2+a3^2) <= 2
   }

RCL_Quaternion_M (XAXIS) { ; Ron Lewen, 76376,2567
   ;  From Pseudocode 10.5 (p. 169) of Pickover's book.
   ;  Looks at Mandelbrot set for a0,a2 plane.
   ;  p1 selects slice in to look at.  p1 should
   ;  not be (0,0) (this yields a blank screen!).
   a0=a2=pixel, a1=real(p1), a3=imag(p1):
   savea0=a0^2-a1^2-a2^2-a3^2+pixel,
   savea2=2*a0*a2+pixel, a0=savea0, a2=savea2,
   (a0^2+a1^2+a2^2+a3^2) <= 2
   }

REB004A = {; Ron Barnett [70153,1233]
   ; try p1 = 0.9, p2 = 2, fn1 = sin, fn2 = cos 
   z = pixel:
   z =p1*fn1(z) + p1*p1*fn2(p2*z) + pixel, |z| <= 100
   }

REB004B = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = 3
   z = pixel:
   z = pixel + p1*(z/2 + z*z/6 + z*z*z/12), |z| <= 100
   }

REB004C = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = 3, p2 = (-0.009,1.225)
   z = pixel:
   z = p2 + p1*(z/2 + z*z/6 + z*z*Z/12), |z| <= 100
   }

REB004D = {; Ron Barnett [70153,1233]
   ; try p1 = -1, fn1 = sin 
   z = pixel:
   z = pixel + fn1(2*z+1)/(2*z+p1), |z| <= 100
   }

REB004E = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = -1, p2 = -1, fn1 = sin, fn2 = cos
   z = pixel:
   z = pixel + fn1(2*z+1)/(2*z+p1); 
   z = z + fn2(4*z+1)/(4*z+p2), |z| <= 100
   }

REB004F = {; Ron Barnett [70153,1233]
   ; try p1 = -1, p2 = (-0.92, 0.979), fn1 = sin 
   z = pixel:
   z = p2 + fn1(2*z+1)/(2*z+p1), |z| <= 100
   }

REB004G = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = -1, p2 = (0.849,0.087), fn1 = sin, fn2 = cos
   z = pixel:
   z = p2 + fn1(2*z+1)/(2*z+p1); 
   z = z + fn2(4*z+1)/(4*z+p1), |z| <= 100
   }

REB004H = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try fn1 = sqr
   z = pixel:
   z = pixel + fn1(3/z - z/4), |z| <= 100
   }

REB004I = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = (-1.354, 0.625) fn1 = sqr
   z = pixel:
   z = p1 + fn1(3/z - z/4), |z| <= 100
   }

REB004J = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try fn1 = tan
   z = pixel:
   x = flip(pixel + fn1(3/z - z/4));
   z = x*z + pixel, |z| <= 100
   }

REB004K = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = (-0.564, 0.045), fn1 = tan
   z = pixel:
   x = flip(pixel + fn1(3/z - z/4));
   z = x*z + p1, |z| <= 100
   }

REB004L = {; Ron Barnett [70153,1233] 
              ; floating point required
   ; try p1 = 1, p2 - 2, fn1 = tan
   z = pixel:
   x = flip(pixel + fn1(p1/z - z/(p2+1)));
   z = x*z + pixel, |z| <= 100
   }

REB004M = {; Ron Barnett [70153,1233] 
              ; floating point required
   ;try p1 = (0.4605, 0.8), fn1 = tan, fn2 = cos
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1(const - 12*x)*x/(4*const);
   y1 = -fn2(const + 12*x)*y/(4*const);
   x2 = x1*x1 - y1*y1 + p1;
   y2 = 2*x*y;
   z = x2 + flip(y2), |z| <= 100
   }   

REB004N = {; Ron Barnett [70153,1233]
   z = 0.5:
   x = pixel*(z - 1/z) + p1,
   z = pixel*(x - 1/sqr(x) + p2), |z| <= 100
   }

REB005A	= {; Ron Barnett [70153,1233]
              ; floating point required
   ; try p1 = 0.77, fn1 = ident, fn2 = ident
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1(const - 12*x)*x/(4*const);
   y1 = -fn2(const + 12*y)*y/(4*const);
   x2 = x1*x1 - y1*y1 + p1;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   } 

REB005B = {; Ron Barnett [70153,1233]
              ; floating point required
   ; try p1 = 0.01, fn1 = ident, fn2 = ident
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1(const - x)*x/const;
   y1 = -fn2(const + y)*y/const;
   x2 = x1*x1 - y1*y1 + p1;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   }

REB005C = {; Ron Barnett [70153,1233]
              ; floating point required
   ; try p1 = -0.5, p2 = -0.1, fn1 = ident, fn2 = ident
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1(const + p1*x)*x/const;
   y1 = -fn2(const + y)*y/const;
   x2 = x1*x1 - y1*y1 + p2;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   } 

REB005D = {; Ron Barnett [70153,1233]
              ; floating point required
   ; try p1 = -1, p2 = -1, fn1 = sin, fn2 = ident
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1((const + p1*x)*x/const);
   y1 = -fn2((const + y)*y/const);
   x2 = x1*x1 - y1*y1 + p2;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   }

REB005E = {; Ron Barnett [70153,1233]
              ; floating point required
   ; try p1 = (0,0.09), fn1 = sin, fn2 = tan
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1((const - x)*x/const);
   y1 = -fn2((const + y)*y/const);
   x2 = x1*x1 - y1*y1 + p1;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   }

REB005G = {; Ron Barnett [70153,1233]
              ; floating point required
   ; try fn1 = ident, fn2 = sin
   z = pixel:
   x = real(z), y = imag(z);
   const = x*x + y*y;
   x1 = -fn1(const + p1*x)*y/const;
   y1 = -fn2(const + y)*x/const;
   x2 = x1*x1 - y1*y1 + p2;
   y2 = 2*x1*y1;
   z = x2 + flip(y2), |z| <= 100
   }

REBRefInd1 = {; Ron Barnett [70153,1233]
   ; Use floating point
   ; p1 = 1, p2 = 2, fn1 = sin, fn2 = sqr
   z = pixel:
   z = (z*z-p1)/(z*z+p2)*fn1(z)*fn2(z) + pixel,
   |z| <= 100
   }

REBRefInd2 = {; Ron Barnett [70153,1233]
   ; Use floating point
   ; try p1 = (0.489, 0.844), fn1 = sin, fn2 = sqr
   z = pixel:
   z = (z*z-1)/(z*z+2)*fn1(z)*fn2(z) + p1,
   |z| <= 100
   }

REBRefInd3 = {; Ron Barnett [70153,1233]
   ; Use floating point
   ; p1 = (0.48, 0.67), fn1 = sin
   z = pixel:
   z = (z*z-1)/(z*z+2)*fn1(z) + p1,
   |z| <= 100
   }

REBRefInd4 = {; Ron Barnett [70153,1233]
   ; Use floating point
   ; try p1 = 1, p2 = 2, fn1 = cosh, fn2 = sqr
   z = pixel:
   z = flip(z);
   z = (z*z-p1)/(z*z+p2)*fn1(z)*fn2(z) + pixel,
   |z| <= 100
   }

REBRefInd5 = {; Ron Barnett [70153,1233]
   ; Use floating point
   ; try p1 = (0.46, 0.482), fn1 = cosh, fn2 = sqr
   z = pixel:
   z = flip(z);
   z = (z*z-1)/(z*z+2)*fn1(z)*fn2(z) + p1,
   |z| <= 100
   }

RecipIke = {; Ron Barnett [70153,1233]
   ; try p1 = (-1.44,-0.4) with royal.map
   z = pixel:
   z = 1/(z*z*z + (p1-1)*z - p1),
   |z| <= 4
   }

quadrants {
  ; floating point is recommended
  z=0, c=pixel,
  r1=(0.0,1.0), r2=(-1.0,0.0), r3=(0.0,-1.0), r4=1:
  z=sqr(z)+c,
  x=real(z), y=imag(z),
  xp=(0 < x), xn=(x < 0), yp=(0 < y), yn=(y < 0),
  k1=xp*yp, k2=xn*yp, k3=xn*yn, k4=xp*yn,
  k=k1*r1+k2*r2+k3*r3+k4*r4,
  c=c+k*p1/z,
  |z| <= 4
  }

Sam_0(XAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z^z - pixel
   }

Sam_1(XAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z^(-z) - pixel
   }

Sam_10(XYAXIS) = {; from SAM.FRM
   z = Pixel:
   z = sin(1/z)
   }

Sam_11(XAXIS) = {; from SAM.FRM
   ;Try this with periodicity=none command line
   z = Pixel:
   z = sinh(1/z)
   }

Sam_2(XAXIS) = {; from SAM.FRM
   ; use integer math, not floating point or you will get a blank screen
   z = Pixel:
   z = z^(1/z) - pixel
   }

Sam_3(XAXIS) = {; from SAM.FRM
   z = Pixel:
    z = z^z^z - pixel
   }

Sam_4(XAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z^(z^(1/z)) - pixel
   }

Sam_5(XAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z^2.718281828 + pixel
   }

Sam_6(XYAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z*cos(z) - pixel
   }

Sam_7(XAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z*sin(z) - pixel
   }

Sam_8 = {; from SAM.FRM
   ;fix by Ron Barnett [70153,1233]   
   z = c = Pixel:
   z = z^c
   }

Sam_9(XYAXIS) = {; from SAM.FRM
   z = Pixel:
   z = z*tanh(z)
   }

ScottLPC(XAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = log(z)+cosxx(z),
  |z|<TEST
  }

ScottLPS(XAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = log(z)+sin(z),
  |z|<TEST
  }

ScottLTC(XAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = log(z)*cosxx(z),
  |z|<TEST
  }

ScottLTS(XAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = log(z)*sin(z),
  |z|<TEST
  }

ScottSIC(XYAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = sqr(1/cosxx(z)),
  |z|<TEST
  }

ScSkCosH(XYAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = cosh(z) - sqr(z),
  |z|<TEST
  }

ScSkLMS(XAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = log(z) - sin(z),
  |z|<TEST
  }

ScSkZCZZ(XYAXIS) {; Lee Skinner [75450,3631]
  z = pixel, TEST = (p1+3):
  z = (z*cosxx(z)) - z,
  |z|<TEST
  }

Silverado(XAXIS) {; Rollo Silver [71174,1453]
   ; Use floating point.
   ; Select p1 such that 0. <= p1 <= 1.
   z = Pixel, zz=z*z, zzz=zz*z, z = (1.-p1)*zz + (p1*zzz),
   test = (p2+4)*(p2+4):
   z = z + Pixel, zsq = z*z,
   zcu = zsq*z, z = (1.-p1)*zsq + p1*zcu,
   |z| <= test
   }

Silverado2 { ; Rollo Silver [71174,1453]
  ; Use floating point.
  st=1-p1,zz=pixel*pixel,z=zz*pixel*real(p1)+zz*real(st):
   z=z+pixel,
   zz=sqr(z), ; and save mod in lastsqr
   z=zz*z*real(p1)+zz*real(st),
    4 > lastsqr
   }

SinEgg(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
    ; Generalized by Tobey J. E. Reed [76437,375]
    ; Try p1=0, p2=4, fn1=sin, fn2=sqr
    ; Try p1=0, p2=4, fn1=sinh, fn2=sqr
    ; Use floating point.
    z  = p1, x  = |z|:
    (1 < x) * (z=fn1(z) + pixel),
    z  = fn2(z)+pixel, x  = |z|,
    x <= p2
   }

SinEggC(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
   ; Generalized by Tobey J. E. Reed [76437,375]
   ; Try p1=0, p2=4, fn1=sinh, fn2=sqr
   ; Try p1=0, p2=4, fn1=sin, fn2=sqr
   ; Use floating point.
   z=p1, x=|z|:
   (z=fn1(z)+pixel)*(1<x)+(z=z)*(x<=1),
   z=fn2(z)+pixel, x=|z|,
   x<=p2
   }

SinInvZ(XYAXIS) = {
   z=pixel, inv=1/pixel+p1:
   z=sin(inv/z),
   |z|<=4
   }

SinhInvZ(XYAXIS) = {
   z=pixel, inv=1/pixel+p1:
   z=sinh(inv/z),
   |z|<=4
   }

Something (xaxis) = {
   z = pixel:
   z = pixel + z*z + 1/z/z,
   |z| <= 4
   }

Somethingelse (xyaxis) = {
   z = 1:
   z = pixel * (z*z + 1/z/z),
   |z| <= 1000000
   }

SymmIcon {; Darell Shaffer [76040,2017]
   z = P1, x = P2,
   bar = (1,-1),
   l = real(P1), a = imag(P2),
   b = .2, g = .1, w = 0, n = 5:
   zbar = z*bar;
   z = ((l +(a *z *zbar) +(b *real(z^n)) +(w *i)) *z) +g *(zbar^(n-1)) +pixel;
   }

SymmIconFix {; Darell Shaffer [76040,2017]
   ; Fix by Jonathan Osuch [73277,1432]
   z = P1, x = P2,
   l = real(P1), a = imag(P2),
   b = .2, g = .1, w = 0, n = 5:
   zbar = conj(z);
   z = ((l +(a *z *zbar) +(b *real(z^n)) +(w *i)) *z) +g *(zbar^(n-1)) +pixel;
   }

TanInvZ(XYAXIS) = {
   z=pixel, inv=1/pixel+p1:
   t=inv/z,
   z=sin(t)/cos(t),
   |z|<=4
   }

TanhInvZ(XYAXIS) = {
   z=pixel, inv=1/pixel+p1:
   z=tanh(inv/z),
   |z|<=4
   }

test {; Michael Theroux [71673,2767]
   ;fix and generalization by Ron Barnett [70153,1233]
   ;=phi
   ; try p1 = 2.236067977 for the golden mean
   z = ((p1 + 1)/2)/pixel:
   z =  z*z + pixel*((p1 + 1)/2),
   |z| <= 4;
   }

test1 {; Michael Theroux [71673,2767]
   ;fix and generalization by Ron Barnett [70153,1233]
   ;=phi
   ; try p1 = 2.236067977 for the golden mean
   c = pixel,
   z = ((p1 + 1)/2):
   z =  z*z + pixel*((p1 + 1)/2) + c,
   |z| <= 4;
   }

test2 {; Michael Theroux [71673,2767]
   ;fix and generalization by Ron Barnett [70153,1233]
   ;=phi
   ; try p1 = 2.236067977 for the golden mean
   z = ((p1 + 1)/2)/pixel:
   z =  z*z*z + pixel*((p1 + 1)/2),
   |z| <= 4;
   }

test3 {; Michael Theroux [71673,2767]
   ;fix and generalization by Ron Barnett [70153,1233]
   ;=phi
   ; try p1 = 2.236067977 for the golden mean
   z = ((p1 + 1)/2)/pixel:
   z =  z*z + pixel*((p1 + 1)/2)/((p1 - 1)/2),
   |z| <= 4;
   }

testm {
  ; Try p1=0.25 and p2=0.15 with float=y or potential=255/800/255
  z = 0, c=pixel:
  z = sqr(z)+c,
  c=c+(p1 * (|z| <= p2)),
  |z| <= 4
  }

TestSinMandC(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
    ; Generalized by Tobey J. E. Reed [76437,375]
    ; Try: p1=4, fn1=sin, fn2=sqr
    z  = p1, x  = |z|:
    (z  = fn1(z)) * (1<x)+(z=z) * (x<=1),
    (z  = fn2(z)+pixel),
    x  = |z|,
    x <= p1
  }

TjerCGhalley (XYAXIS) {; Chris Green -- Halley's formula
  ; Modified by Tobey J. E. Reed [76437,375]
  ; P1 usually 1 to 1.5, P2 usually zero. Use floating point.
  z=(1,1):
   z5=z*z*z*z*z,
   z6=z*z5, z7=z*z6,
   z=z-p1*((z7-z+pixel)/ ((p1*z6-3)-(8.0*z5)*(z7+z-pixel)/(3.30*z6-12))),
    0.0001 <= |z7-z-pixel|
  }

TjerCubic (XYAXIS) {; Lee Skinner [75450,3631]
  ; Modified by Tobey J. E. Reed [76437,375]
  p = pixel, test = p1 + 3,
  t3 = 5*p, t2 = p*p,
  a = (t2 + 1)/t3+t2, b = 3.149*a*a*a + (t2 - 5)/t2,
  aa3 = a*a*p1, z = 0 - a :
   z = z*z - aa3*z + a,
    |z| < test
 }

TjerDeltaLog(XAXIS) {; Mark Peterson
  ; Modified by Tobey J. E. Reed [76437,375]
  z = pixel, c = log(pixel):
   z = cosh(z) + c/2,
    |z| <= 4
  }

TjerDragon {; Mark Peterson
  ; Modified by Tobey J. E. Reed [76437,375]
  z = Pixel:
   z = tan(z) + (-0.74543, 0.2),
    |z| <= 4
  }

TjerEnt {; Scott Taylor
  ; Modified by Tobey J. E. Reed [76437,375]
  ; Try params=.5/.75 and the first function as exp.
  ; Zoom in on the swirls around the middle.  There's a
  ; symmetrical area surrounded by an asymmetric area.
  z = Pixel, y = fn1(z)+p1, base = log(p1):
   z = y * 3.1416 * log(z)/base,
    |z| <= 5
  }

TjerFzppfnpo  {; Lee Skinner [75450,3631]
  ; Modified by Tobey J. E. Reed [76437,375]
  z = pixel, f = 2*(pixel)^(pixel):
   z = fn1(z) + f,
    |z| <= 50
  }

TjerFzppfnre  {; Lee Skinner [75450,3631]
  ; Modified by Tobey J. E. Reed [76437,375]
  z = pixel, f = 1./(pixel):
   z = fn1(z) + f * p1,
    |z| <= 50
  }

TjerHyperMandel {; Chris Green.
  ; Modified and Generalized by Tobey J. E. Reed [76437,375]
  ; A four dimensional version of the mandelbrot set.
  ; Use P1 to select which two-dimensional plane of the
  ; four dimensional set you wish to examine.
  ; Use floating point.
  a=(0,0),b=(0,0):
   z=z+1, anew=fn1(a)-fn1(b)+pixel,
   b=3.17*a*b-p1, a=anew,
    |a|+|b| <= 4
  }

TjerInvMandel (XAXIS) {; Mark Peterson
  ; Modified by Tobey J. E. Reed [76437,375]
  c = z = 1 / pixel:
   z = cos(z) + 2*c;
    |z| <= 4
  }

TjerMandelTangent {; Fractal Creations example (revised for v.16)
  ; Modified by Tobey J. E. Reed [76437,375]
  z = pixel:
   z = pixel * tan(z) * 3.14159 * p1,
    |real(z)| < 32
  }

TjerMTet (XAXIS) {;Mandelbrot form 1 of the Tetration formula -- Lee Skinner  ; Modified and Generalized by Tobey J. E. Reed [76437,375]
  z = pixel:
   z = (pixel ^ z + pixel) + fn1(pixel),
    |z| <= (P1 + 3)
  }

TjerNewton4(XYAXIS) {; Mark Peterson
  ; Modified by Tobey J. E. Reed [76437,375]
  z = pixel, Root = 1:
   z3 = z*z*z,
   z4 = z3 * z,
   z = (3 / z4 - Root) / (6 * z3),
    .004 <= |z4 - Root|
  }

TjerNewtonSinExp (XAXIS) {; Chris Green
  ; Generalized by Tobey J. E. Reed [76437,375]
  ; Newton's formula applied to sin(x)+exp(x)-1=0.
  ; Use floating point.
  z=pixel:
   z1=exp(z),
   z2=sin(z)+z1-1,
   z=z-p1*z2/(fn1(z)-z1),
    .0001 < |z2|
  }

TLog (XAXIS) = {; Lee Skinner [75450,3631]
   z = c = log(pixel):
   z = c ^ z,
   z <= (p1 + 3)
   }

Tobey3(XAXIS) = {
   z = pixel:
   c = pixel - sqr(z),
   c = pixel + c/z,
   z = c - z * pixel,
   |z| < 4
   }

TobeyCGNewton3 {; Chris Green -- A variation on newton iteration.
  ; Modified and Generalized by Tobey J. E. Reed [76437,375]
  ; The initial guess is fixed at (1,1), but the equation solved
  ; is different at each pixel ( x^3-pixel=0 is solved).
  ; Use floating point.
  ; Try P1=1.8.
  z=(1,1):
   z2=z*z, z3=z*z2,
   z=z-p1*fn1((z2-pixel)/(2.13*z2)),
    0.0001 < |z3-pixel|
  }

TobeyHalley (XYAXIS) {; Chris Green. Halley's formula applied to x^7-x=0.
  ; Modified and Generalized by Tobey J. E. Reed [76437,375]
  ; P1 usually 1 to 1.5, P2 usually zero. Use floating point.
  ; Setting P1 to 1 creates the picture on page 277 of Pickover's book
  z=pixel:
   z5=z*z*z*z*z, z6=fn1(z*z5),
   z7=fn2(z*z6),
   z=fn2(z-p1*((z7-z))/ (fn1((7.0*z6-1)-(42.0*z5)*(z7-z)/(14.0*z6-2)))),
    0.0001 <= |z7-z|
  }

TobeyHalleySin (XYAXIS) {; Chris Green. Halley's formula applied to sin(x)=0.
  ; Generalized by Tobey J. E. Reed [76437,375]
  ; Use floating point.
  ; P1 = 0.1 will create the picture from page 281 of Pickover's book.
  z=pixel:
   s=fn1(z), c=fn2(z)
   z=z+p1*(s/(c-(s-s)/(c*c))),
    0.0001 <= |s|
  }

TobeyLeeMandel1(XYAXIS) {; Kevin Lee
  ; Generalized by Tobey J. E. Reed [76437,375]
  z=Pixel:
   c=fn1(pixel)/z,
   c=z+2*c,
   z=fn2(z+1),
    |z|<4
  }

TobeyLeeMandel2(XYAXIS) {; Kevin Lee
  ; Generalized by Tobey J. E. Reed [76437,375]
  z=Pixel:
   c=fn1(pixel)/z,
   c=z+c,
   z=fn2(c*pixel),
    |z|<4
   }

TobeyLeeMandel3(XAXIS) {; Kevin Lee
  ; Generalized by Tobey J. E. Reed [76437,375]
  z=Pixel, c=Pixel-fn1(z):
   c=Pixel+c/z,
   z=c-fn2(z*pixel),
    |z|<4
  }

TobeyMyFractal {; Fractal Creations example
  ; Generalized by Tobey J. E. Reed [76437,375]
  c = z = 1/pixel:
   z = fn1(z) + c/p1,
    |z| <= 4
  }

TobeyPsudoMandel(XAXIS) {; davisl - try center=0,0/magnification=28
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = Pixel:
   z = ((z/2.7182818)^z)*fn1(6.2831853*z) + pixel,
    |z| <= 4
  }

TobeyRichard1 (XYAXIS) {; Jm Richard-Collard
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = pixel:
   sq=z*z, z=(sq*fn1(sq)+sq)+pixel,
    |z|<=50
  }

TobeyRichard2 (XYAXIS) {; Jm Richard-Collard
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = pixel:
   z=1/(fn1(z*z+pixel*pixel)),
    |z|<=50
  }

TobeyRichard3 (XAXIS) {; Jm Richard-Collard
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = pixel:
   sh=fn1(z), z=(1/(sh*sh))+pixel,
    |z|<=50
  }

TobeySterling(XYAXIS) {; davisl
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = Pixel:
   z = (fn1((z/2.7182818)^z))/fn2(6.2831853*z),
    |z| <= 4
  }

TobeySterling2(XAXIS) {; davisl
  ; Generalized by Tobey J. E. Reed [76437,375]
  z = Pixel:
   z = ((z/2.7182818)^z)/fn1(6.2831853*z) + pixel,
    |z| <= 4
  }

TobeyWineglass(XAXIS) {; Pieter Branderhorst
  ; Modified and Generalized by Tobey J. E. Reed [76437,375]
  c = z = pixel:
   z = z * z + c,
   c = (1+flip(imag(fn1(c)))) * real(fn1(c)) / 3 + z,
    |z| <= 4 }

TSinh (XAXIS) = {; Lee Skinner [75450,3631]
   z = c = sinh(pixel):
   z = c ^ z,
   z <= (p1 + 3)
   }

TurtleC(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
    ; Generalized by Tobey J. E. Reed [76437,375]
    ; Try p1=0, p2=4, fn1=sqr, fn2=sqr
    ; Note:  use floating point
    z   = p1:
    x   = real(z),
   (z   = fn1(z)+pixel) * (x<0) + (z=fn2(z)-pixel) * (0<=x),
   |z| <= p2
   }

ULI_1 = {; from ULI.FRM
   z = Pixel:
   z = fn1(1/fn2(z)),
   |z| <= 4
   }

ULI_2 = {; from ULI.FRM
   z = Pixel:
   z = fn1(1/fn2(z+p1)),
   |z| <= p2
   }

ULI_3 = {; from ULI.FRM
   z = Pixel:
   z = fn1(1/fn2(z+p1)+p1),
   |z| <= p2
   }

ULI_4 = {; from ULI.FRM
   z = Pixel:
   z = fn1(1/(z+p1))*fn2(z+p1),
   |z| <= p2
   }

ULI_5 = {; from ULI.FRM
   z = Pixel, c = fn1(pixel):
   z = fn2(1/(z+c))*fn3(z+c),
   |z| <= p1
   }

ULI_6 = {; from ULI.FRM
   z = Pixel:
   z = fn1(p1+z)*fn2(p2-z),
   |z| <= p2+16
   }

WaldoTwinsC(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
    ; Generalized by Tobey J. E. Reed [76437,375]
    ; Try p1=0, p2=4, fn1=cosxx, fn2=sin
    ; Note:  use floating point
    z   = p1:
    z   = fn1(fn2(z+pixel)) + pixel,
   |z| <= p2
   }

Whatever_the_name(XAXIS) = {
   z = pixel:
   z=z*z+(1/z*z)+pixel,
   }

z^3-1=0(XAXIS)  {
   ; Advanced Fractal Programming in C  - Stevens
   ; Run with inside = ZMAG to turn off periodicity checking
   x=real(pixel), y=imag(pixel):
   x2 = x*x, y2 = y*y,
   xold = x, yold = y, xmy = x2 - y2,
   d = 3 * (xmy * xmy + 4*x2*y2),
   x = .66666667*x + xmy/d, y = .66666667*y - 2*x*y/d,
   x != xold && y != yold
   }

Ze2 (XAXIS) = {; Lee Skinner [75450,3631]
   s1 = exp(1.,0.),
   s = s1 * s1,
   z = Pixel:
   z = z ^ s + pixel,
   |z| <= 100
   }

Zexpe (XAXIS) = {; Lee Skinner [75450,3631]
   s = exp(1.,0.), z = Pixel:
   z = z ^ s + pixel,
   |z| <= 100
   }

Zexpe2 (XAXIS) = {; Lee Skinner [75450,3631]
   s = exp(1.,0.), z = Pixel:
   z = z ^ s + z ^ (s * pixel),
   |z| <= 100
   }

Zppchco8  {; Lee Skinner [75450,3631]
   z = pixel, f = cosxx (pixel):
   z = cosh (z) + f,
   |z|<=8192
   }
