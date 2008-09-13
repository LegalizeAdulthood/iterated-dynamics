IslandOfChaos(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
    ; Generalized by Tobey J. E. Reed [76437,375]
    ; Try p1=0, p2=4, fn1=sqr, fn2=sin, fn3=cosxx
    ; Note:  use floating point
    z   =  p1, x   =  1:
   (x  <  10)  * (z=fn1(z) + pixel),
   (10 <=  x)  * (z=fn2(z) / fn3(z) + pixel),
    x   = x+1,
   |z| <= p2
   }

IslandOfChaosC(XAXIS_NOPARM) {; Jonathan Osuch [73277,1432]
   ; Generalized by Tobey J. E. Reed [76437,375]
   ; Try p1=0, p2=4, fn1=sqr, fn2=sin, fn3=cos
   ; Note:  use floating point
   z=p1, x=1:
   (z=fn1(z)+pixel)*(x<10)+(z=fn2(z)/fn3(z)+pixel)*(10<=x),
   x=x+1, |z|<=4
   }

j1 {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2,
   |z| <= 4
   }

jc {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2*c,
   |z| <= 4
   }

jfnc {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2*fn1(c),
   |z| <= 4
   }

jfnz {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2*fn1(z),
   |z| <= 4
   }

JMask = {; Ron Barnett [70153,1233]
   ; try p1 = (1,0), p2 = (0,0.835), fn1 = sin, fn2 = sqr
   z = fn1(pixel):
   z = P1*fn2(z)^2 + P2, |z| <= 4
   }

joc {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2/c,
   |z| <= 4
   }

joz {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2/z,
   |z| <= 4
   }

jz   {; from EXPLOD.FRM
   z=pixel, c=p1:
   z=sqr(z)+c,
   c=c+p2*z,
   |z| <= 4
   }

JSomethingelse (xyaxis) = {
   z = pixel:
   z = p1 * (z*z + 1/z/z),
   |z| <= 1000000
   }

J_Lagandre2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (3 * z*z - 1) / 2 + c
   |z| < 100
   }

J_Lagandre3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = z * (5 * z*z - 3) / 2 + c
   |z| < 100
   }

J_Lagandre4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z*z*(35 * z*z - 30) + 3) / 8 + c
   |z| < 100
   }

J_Lagandre5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = z* (z*z*(63 * z*z - 70) + 15 ) / 8 + c
   |z| < 100
   }

J_Lagandre6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z*z*(z*z*(231 * z*z - 315)  + 105 ) - 5) / 16 + c
   |z| < 100
   }

J_Lagandre7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = z* (z*z*(z*z*(429 * z*z - 693) + 315) - 35 ) / 16 + c
   |z| < 100
   }

J_Laguerre2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z*(z - 4) +2 ) / 2 + c,
   |z| < 100
   }

J_Laguerre3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z*(z*(-z + 9) -18) + 6 ) / 6 + c,
   |z| < 100
   }

J_Laguerre4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z * ( z * ( z * ( z - 16)+ 72) - 96)+ 24 ) / 24 + c,
   |z| < 100
   }

J_Laguerre5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z * ( z * ( z * ( z * (-z +25) -200) +600) -600) + 120 ) / 120 + c,
   |z| < 100
   }

J_Laguerre6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = (z *(z *(z *(z *(z*(z -36) +450) -2400) + 5400)-4320)+ 720) / 720 + c,
   |z| < 100
   }

J_TchebychevC2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z-2),
   |z|<100
   }

J_TchebychevC3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z-3),
   |z|<100
   }

J_TchebychevC4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z-4)+2),
   |z|<100
   }

J_TchebychevC5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z-5)+5),
   |z|<100
   }

J_TchebychevC6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z*(z*z-6)+9)-2),
   |z|<100
   }

J_TchebychevC7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z*(z*z-7)+14)-7),
   |z|<100
   }

J_TchebychevS2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z-1),
   |z|<100
   }

J_TchebychevS3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z-2),
   |z|<100
   }

J_TchebychevS4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z-3)+1),
   |z|<100
   }

J_TchebychevS5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z-4)+3),
   |z|<100
   }

J_TchebychevS6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z*(z*z-5)+6)-1),
   |z|<100
   }

J_TchebychevS7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z*(z*z-6)+10)-4),
   |z|<100
   }

J_TchebychevT2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(2*z*z-1),
   |z|<100
   }

J_TchebychevT3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(4*z*z-3),
   |z|<100
   }

J_TchebychevT4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(8*z*z+8)+1),
   |z|<100
   }

J_TchebychevT5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*(z*z*(16*z*z-20)+5)),
   |z|<100
   }

J_TchebychevT6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z*(32*z*z-48)+18)-1),
   |z|<100
   }

J_TchebychevT7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z*(64*z*z-112)+56)-7),
   |z|<100
   }

J_TchebychevU2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(4*z*z-1),
   |z|<100
   }

J_TchebychevU3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(8*z*z-4),
   |z|<100
   }

J_TchebychevU4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(16*z*z-12)+1),
   |z|<100
   }

J_TchebychevU5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(32*z*z-32)+6),
   |z|<100
   }

J_TchebychevU6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*(z*z*(z*z*(64*z*z-80)+24)-1),
   |z|<100
   }

J_TchebychevU7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = pixel, z = P1:
   z = c*z*(z*z*(z*z*(128*z*z-192)+80)-8),
   |z|<100
   }

JuliaConj(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; try real part of p1 = -1.1 and imag part of p1 = .09
   z = pixel:
   z = Sqr(conj(z)) + P1,
   |z| <= 4
   }

JuliConj01(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = -.93, imag(p1) = .3, map = blues
   z = pixel:
   z = Sqr(z) + Conj(P1),
   |z| <= 4
   }

JuliConj02(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = .3, imag(p1) = .25, map = neon
   z = pixel:
   z = Sqr(Conj(z)) + Conj(P1),
   |z| <= 4
   }

JuliConj03 {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = .40, imag(p1) = 0, map = glasses2
   z = pixel:
   z = Sqr(conj(z))*conj(z) + P1,
   |z| <= 4
   }

JuliConj04 {; Paul J. Horn - a conjugate Julia (I think)
   ;Try real(p1) = .53, imag(p1) = .63, map = volcano
   z = pixel:
   z = Sqr(z)*z + Conj(P1),
   |z| <= 4
   }

JuliConj05 {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = .6, imag(p1) = .4, map = chroma
   z = pixel:
   z = Sqr(conj(z))*conj(z) + Conj(P1),
   |z| <= 4
   }

JuliConj06(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = .99, imag(p1) = .72
   z = pixel:
   z = Sqr(Sqr((conj(z)))) + P1,
   |z| <= 4
   }

JuliConj07(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = -.245, imag(p1) = .44, map = royal
   z = pixel:
   z = Sqr(Sqr(z)) + Conj(P1),
   |z| <= 4
   }

JuliConj08(Origin) {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = -1, imag(p1) = .11, map = blues
   z = pixel:
   z = Sqr(Sqr((conj(z)))) + Conj(P1),
   |z| <= 4
   }

JuliConj09 {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = -.677, imag(p1) = .333, real(p2) = 9, map = blues
   z = pixel:
   z = (conj(z))^P2 + P1,
   |z| <= 4
   }

JuliConj10 {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = .1005, imag(p1) = .68, real(p2) = 5, map = chroma
   z = pixel:
   z = (z)^P2 + Conj(P1),
   |z| <= 4
   }

JuliConj11 {; Paul J. Horn - a conjugate Julia (I think)
   ; Try real(p1) = -.37, imag(p1) = .6, real(p2) = 6, map = volcano
   z = pixel:
   z = (conj(z))^P2 + Conj(P1),
   |z| <= 4
   }

JulibrotSlice1 = {; Randy Hutson - 2D slice of 4D Julibrot
  z = real(p1)+flip(imag(pixel)), c = real(pixel)+flip(imag(p1)):
  z = sqr(z)+c,
  LastSqr <= 4
  }

LambdaPwr {; Ron Barnett [70153,1233]
   ; try p1 = (0.75,0.75), p2 = (2.5,0)
   z = pixel:
   z = p1*z*(1 - z^p2),
   |z| <= 100
   }

Leeze (XAXIS) = {; Lee Skinner [75450,3631]
   s = exp(1.,0.), z = Pixel, f = Pixel ^ s:
   z = cosxx (z) + f,
   |z| <= 50
   }

Liar1 { ; by Chuck Ebbert. [76306,1226]
   ; X: X is as true as Y
   ; Y: Y is as true as X is false
   ; Calculate new x and y values simultaneously.
   ; y(n+1)=abs((1-x(n) )-y(n) ), x(n+1)=1-abs(y(n)-x(n) )
   z = pixel:
   z = 1 - abs(imag(z)-real(z) ) + flip(1 - abs(1-real(z)-imag(z) ) ),
   |z| <= 1
   }

Liar2 { ; by Chuck Ebbert. [76306,1226]
   ; Same as Liar1 but uses sequential reasoning, calculating
   ;  new y value using new x value.
   ; x(n+1) = 1 - abs(y(n)-x(n) );
   ; y(n+1) = 1 - abs((1-x(n+1) )-y(n) );
   z = pixel:
   x = 1 - abs(imag(z)-real(z)),
   z = flip(1 - abs(1-real(x)-imag(z) ) ) + real(x),
   |z| <= 1
   }

M-SetInNewton(XAXIS) {; use float=yes
   ; jon horner 100112,1700, 12 feb 93
   z = 0,  c = pixel,  cminusone = c-1:
   oldz = z,
   nm = 3*c-2*z*cminusone,
   dn = 3*(3*z*z+cminusone),
   z = nm/dn+2*z/3,
   |(z-oldz)|>=|0.01|
   }

m1 {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1,
   |z| <= 4
   }

MandelConj(XAXIS) {; Paul J. Horn , this was mentioned in Pickover's book
    ; Computers, Chaos, Patterns and Beauty.  He didn't give the forumula, so
    ; I came up with this
    z = c = Pixel:
    z = Sqr(conj(z)) + Pixel,
    |z| <= 4
    }

MandConj01(XAXIS) {; Paul J. Horn, see MandelConj.
    ; This is a variation on a theme.
    z = c = Pixel:
    z = Sqr(z) + Conj(Pixel),
    |z| <= 4
    }

MandConj02(XAXIS) {; Paul J. Horn, see MandelConj.
    ; Another variation on the theme.
    z = c = Pixel:
    z = Sqr(Conj(z)) + Conj(Pixel),
    |z| <= 4
    }

MandConj03(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr(conj(z))*conj(z) + Pixel,
    |z| <= 4
    }

MandConj04(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr((z))*(z) + Conj(Pixel),
    |z| <= 4
    }

MandConj05(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr(conj(z))*conj(z) + Conj(Pixel),
    |z| <= 4
    }

MandConj06(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr(Sqr(conj(z))) + Pixel,
    |z| <= 4
    }

MandConj07(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr(Sqr((z))) + Conj(Pixel),
    |z| <= 4
    }

MandConj08(XAXIS) {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = Sqr(Sqr(conj(z))) + Conj(Pixel),
    |z| <= 4
    }

MandConj09  {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = (conj(z))^p1 + Pixel,
    |z| <= 4
    }

MandConj10  {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = z^p1 + Conj(Pixel),
    |z| <= 4
    }

MandConj11 {; Paul J. Horn
    ; yet another variation on the theme
    z = c = Pixel:
    z = (conj(z))^p1 + Conj(Pixel),
    |z| <= 4
    }

MandellambdaPwr {; Ron Barnett [70153,1233]
   ; This provide a "map" for LambdaPwr
   z = (1/(p1+1))^(1/p1):
   z = pixel*z*(1 - z^p1),
   |z| <= 100
   }

Mask = {; Ron Barnett [70153,1233]
   ; try fn1 = log, fn2 = sinh, fn3 = cosh
   ;P1 = (0,1), P2 = (0,1)
   ;Use floating point
   z = fn1(pixel):
   z = P1*fn2(z)^2 + P2*fn3(z)^2 + pixel,
   |z| <= 4
   }

mc {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1*c,
   |z| <= 4
   }

mfnc {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1*fn1(c),
   |z| <= 4
   }

mfnz {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1*fn1(z),
   |z| <= 4
   }

Michaelbrot {; Michael Theroux [71673,2767]
   ; Fix and generalization by  Ron Barnett [70153,1233]   
   ; Try p1 = 2.236067977 for the golden mean
   ;based on Golden Mean
   z = pixel:
   z = sqr(z) + ((p1 + 1)/2),
   |z| <= 4
   }

moc {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1/c,
   |z| <= 4
   }

Mothra (XAXIS) { ; Ron Lewen, 76376,2567
   ; Remember Mothra, the giant Japanese-eating moth?
   ; Well... here he (she?) is as a fractal!   ;
   z=pixel:
   z2=z*z, z3=z2*z, z4=z3*z,
   a=z4*z + z3 + z + pixel, b=z4 + z2 + pixel,
   z=b*b/a,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

moz {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1/z,
   |z| <= 4
   }

mz {; from EXPLOD.FRM
   z=0, c=pixel:
   z=sqr(z)+c,
   c=c+p1*z,
   |z| <= 4
   }

M_Lagandre2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (3 * z*z - 1) / 2 + c
   |z| < 100
   }

M_Lagandre3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = z * (5 * z*z - 3) / 2 + c
   |z| < 100
   }

M_Lagandre4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z*z*(35 * z*z - 30) + 3) / 8 + c
   |z| < 100
   }

M_Lagandre5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = z* (z*z*(63 * z*z - 70) + 15 ) / 8 + c
   |z| < 100
   }

M_Lagandre6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z*z*(z*z*(231 * z*z - 315)  + 105 ) - 5) / 16 + c
   |z| < 100
   }

M_Lagandre7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = z* (z*z*(z*z*(429 * z*z - 693) + 315) - 35 ) / 16 + c
   |z| < 100
   }

M_Laguerre2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z*(z - 4) +2 ) / 2 + c,
   |z| < 100
   }

M_Laguerre3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z*(z*(-z + 9) -18) + 6 ) / 6 + c,
   |z| < 100
   }

M_Laguerre4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z * ( z * ( z * ( z - 16)+ 72) - 96)+ 24 ) / 24 + c,
   |z| < 100
   }

M_Laguerre5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z * ( z * ( z * ( z * (-z +25) -200) +600) -600) + 120 ) / 120 + c,
   |z| < 100
   }

M_Laguerre6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = (z *(z *(z *(z *(z*(z -36) +450) -2400) +5400) -4320) +720) / 720 + c,
   |z| < 100
   }

M_TchebychevC2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z-2),
   |z|<100
   }

M_TchebychevC3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z-3),
   |z|<100
   }

M_TchebychevC4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z-4)+2),
   |z|<100
   }

M_TchebychevC5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z-5)+5),
   |z|<100
   }

M_TchebychevC6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z*(z*z-6)+9)-2),
   |z|<100
   }

M_TchebychevC7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z*(z*z-7)+14)-7),
   |z|<100
   }

M_TchebychevS2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z-1),
   |z|<100
   }

M_TchebychevS3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z-2),
   |z|<100
   }

M_TchebychevS4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z-3)+1),
   |z|<100
   }

M_TchebychevS5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z-4)+3),
   |z|<100
   }

M_TchebychevS6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z*(z*z-5)+6)-1),
   |z|<100
   }

M_TchebychevS7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z*(z*z-6)+10)-4),
   |z|<100
   }

M_TchebychevT2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(2*z*z-1),
   |z|<100
   }

M_TchebychevT3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(4*z*z-3),
   |z|<100
   }

M_TchebychevT4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(8*z*z+8)+1),
   |z|<100
   }

M_TchebychevT5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*(z*z*(16*z*z-20)+5)),
   |z|<100
   }

M_TchebychevT6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z*(32*z*z-48)+18)-1),
   |z|<100
   }

M_TchebychevT7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z*(64*z*z-112)+56)-7),
   |z|<100
   }

M_TchebychevU2 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(4*z*z-1),
   |z|<100
   }

M_TchebychevU3 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(8*z*z-4),
   |z|<100
   }

M_TchebychevU4 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(16*z*z-12)+1),
   |z|<100
   }

M_TchebychevU5 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(32*z*z-32)+6),
   |z|<100
   }

M_TchebychevU6 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*(z*z*(z*z*(64*z*z-80)+24)-1),
   |z|<100
   }

M_TchebychevU7 {; Rob den Braasem [rdb@KTIBV.UUCP]
   c = P1, z = Pixel:
   z = c*z*(z*z*(z*z*(128*z*z-192)+80)-8),
   |z|<100
   }

Natura {; Michael Theroux [71673,2767]
   ; Fix and generalization by  Ron Barnett [70153,1233]   
   ;phi yoni
   ; try p1 = 2.236067977 for the golden mean
   z = pixel:
   z = z*z*z + ((p1 + 1)/2)
   |z| <= 4
   }

Newducks(XAXIS) = {
   z=pixel,t=1+pixel:
   z=sqr(z)+t,
   |z|<=4
   }

non-conformal {; Richard Hughes (Brainy Smurf) [70461,3272]
   ; From Media Magic Calender - August
   z=x=y=x2=y2=0:
   t = x * y,
   x = x2 + t + real(pixel), y = y2 - t + imag(pixel),
   x2 = sqr(x), y2 = sqr(y), z=x + flip(y),
   |z| <= 4
   }

No_name(xaxis) = {
   z = pixel:
   z=z+z*z+(1/z*z)+pixel,
   |z| <= 4
   }

OldCGNewtonSinExp (XAXIS) {; Chris Green
   ; For images using old incorrect cos function
   ; Use floating point.
   z=pixel:
   z1=exp(z),
   z2=sin(z)+z1-z,
   z=z-p1*z2/(cosxx(z)+z1),
   .0001 < |z2|
   }

OldHalleySin (XYAXIS) {; Chris Green
   ; For images using old incorrect cos function
   ; Use floating point.
   z=pixel:
   s=sin(z),
   c=cosxx(z),
   z=z-p1*(s/(c-(s*s)/(c+c))),
   0.0001 <= |s|
   }

OldManowar (XAXIS) {; Lee Skinner [75450,3631]
   z0 = 0, z1 = 0, test = p1 + 3, c = pixel :
   z = z1*z1 + z0 + c,
   z0 = z1,
   z1 = z,
   |z| < test
   }

OldNewtonSinExp (XAXIS) {; Chris Green
   ; Newton's formula applied to sin(x)+exp(x)-1=0.
   ; For images using old incorrect cos function
   ; Use floating point.
   z=pixel:
   z1=exp(z), z2=sin(z)+z1-1
   z=z-p1*z2/(cosxx(z)+z1),
   .0001 < |z2|
   }

phoenix_j (XAXIS) {; Richard Hughes (Brainy Smurf) [70461,3272]
   ; Use P1=0.56667/-0.5   &   .1/.8
   ; Use floating point.
   x=real(pixel), y=imag(pixel), z=nx=ny=x1=x2=y1=y2=0:
   x2 = sqr(x), y2 = sqr(y),
   x1 = x2 - y2 + real(p1) + imag(p1) * nx,
   y1 = 2 * x * y + imag(p1) * ny,
   nx=x, ny=y, x=x1, y=y1, z=nx + flip(ny),
   |z| <= 4
   }

phoenix_m {; Richard Hughes (Brainy Smurf) [70461,3272]
   ; Mandelbrot style map of the Phoenix curves
   ; Use floating point.
   z=x=y=nx=ny=x1=y1=x2=y2=0:
   x2 = sqr(x), y2 = sqr(y),
   x1 = x2 - y2 + real(pixel) + imag(pixel) * nx,
   y1 = 2 * x * y + imag(pixel) * ny,
   nx=x, ny=y, x=x1, y=y1, z=x + flip(y),
   |z| <= 4
   }

PolyGen = {; Ron Barnett [70153,1233]
   ;p1 must not be zero
   ;zero can be simulated with a small
   ;value for p1
   ;use floating point
   ;try p1 = 1 and p2 = 0.3
   z=(-p2+(p2*p2+(1-pixel)*3*p1)^0.5)/(3*p1):
   z=p1*z*z*z+p2*z*z+(pixel-1)*z-pixel,
   |z| <= 100
   }

PseudoLambda {; Ron Barnett [70153,1233]
   ; Use floating point.
   ; try p1 = (-1,0.45), p2 = (1,0)
   z = pixel:
   x = real(z), y = imag(z),
   x1 = -p1*(x - x*x + y*y) + p2,
   y = -p1*(y - 2*x*y),
   z = x1 + flip(y),
   |z| <= 100
   }

PseudoMandelLambda {; Ron Barnett [70153,1233]
   ; Use floating point.
   z = 0.5, c = pixel:
   x = real(z), y = imag(z),
   x1 = -c*(x - x*x + y*y) + p1,
   y = -c*(y - 2*x*y),
   z = x1 + flip(y),
   |z| <= 100
   }

PseudoZeePi = {; Ron Barnett [70153,1233]
   ; try p1 = 0.1, p2 = 0.39
   z = pixel:
   x = 1-z^p1;
   z = z*((1-x)/(1+x))^(1/p1) + p2,
   |z| <= 4
   }

Ramanujan1(ORIGIN) = {
   z = pixel:
   z = (cosh(p1 * sqr(z)) - sinh(p2 * sqr(z))/(p2 * sqr(z)))/z,
   |z|<= 4
   }

Raphaelbrot {; Michael Theroux [71673,2767]
   ; Fix and generalization by  Ron Barnett [70153,1233]   
   ;phi
   ; try p1 = 2.236067977 for the golden mean 
   z = pixel:
   z = sqr(z) + ((p1 - 1)/2)
   |z| <= 4
   }

RCL_1 (XAXIS) { ; Ron Lewen [76376,2567]
   ;  An interesting Biomorph inspired by Pickover's
   ;  Computers, Pattern, Choas and Beauty.
   ;  Use Floating Point
   z=pixel:
   z=pixel/z-z^2,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_11 { ; Ron Lewen, 76376,2567
  ; A variation on the formula used to generate
  ; Figure 9.18 (p. 134) from Pickover's book.
  ; P1 sets the initial value for z.
  ; Try p1=.75, or p1=2, or just experiment!
  z=real(p1):
    z=z*pixel-pixel/sqr(z)
    z=flip(z),
      abs(z) < 8
  }

RCL_2 (XAXIS) { ; Ron Lewen [76376,2567]
   ;  A biomorph flower?  Simply a change in initial
   ;  conditions from RCL_1 above
   ; Use Floating Point
   z=1/pixel:
   z=pixel/z-z^2
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_3 (XAXIS) { ; Ron Lewen [76376,2567]
   ;  A seemingly endless vertical pattern.  The most activity
   ;  is around the center of the image.
   ;  Use Floating Point
   z=pixel:
   z=pixel^z+z^pixel,
   |real(z)| <= 100 || |imag(z)| <= 100
   }

RCL_4_M (XAXIS) { ; Ron Lewen, 76376,2567
  ; A Mandelbrot-style variation on Pickover's book,
  ; Figure 8.9 (p. 105).
  ; Use floating point
  z=pixel:
    z=sin(z^2) + sin(z) + sin(pixel),
      |z| <= 4
  }

RCL_4_J { ; Ron Lewen, 76376,2567
  ;  A julia-style variation of the formula in Figure 8.9
  ;  (p. 105) of Pickover's book.
  z=pixel:
    z=sin(z^2) + sin(z) + sin(p1),
      |z| <= 4
  }

RCL_5_M (XAXIS) { Ron Lewen, 76376,2567
  ;  A variation on the classical Mandelbrot set
  ;  formula.
  ;  Use floating point
  z=pixel:
    z=sin(z^2+pixel),
      |z| <= 4
  }

RCL_5_J (ORIGIN) { Ron Lewen, 76376,2567
  ;  A variation on the classical Julia set.
  ;  Use floating point
  z=pixel:
    z=sin(z^2+p1),
      |z| <= 4
  }

RCL_6_M (XAXIS) { ; Ron Lewen, 76376,2567
  ;  A variation on the classic Mandelbrot formula
  ;  Use floating point
  z=pixel:
    z=sin(z)^2 + pixel,
      |z| <= 4
  }

RCL_6_J (ORIGIN) { ; Ron Lewen, 76376,2567
  ;  A variation on the classic Julia formula
  ;  use floating point
  z=pixel:
    z=sin(z)^2 + p1,
      |z| <= 4
  }

RCL_7 (XAXIS) { ; Ron Lewen, 76376,2567
  ; Inspired by the Spider
  ; fractal type included with Fractint
  z=c=pixel:
    z=z^2+pixel+c
    c=c^2+pixel+z
      |z| <= 4
  }

RCL_8_M { ; Ron Lewen, 76376,2567
  ;  Another variation on the classic Mandelbrot
  ;  set.
  z=pixel:
    z=z^2+flip(pixel)
      |real(z)| <= 100 || |imag(z)| <= 100
  }

RCL_8_J (ORIGIN) { ; Ron Lewen, 76376,2567
  z=pixel:
    z=z^2+flip(p1)
      |real(z)| <= 100 || |imag(z)| <= 100
  }

RCL_9 (XAXIS) { ; Ron Lewen, 76376,2567
  z=pixel:
    z=(z^2+pixel)/(pixel^2+z)
      |z| <= 4
  }

RCL_10 { ; Ron Lewen, 76376,2567
  z=pixel:
    z=flip((z^2+pixel)/(pixel^2+z))
      |z| <= 4
  }

RCL_12 (XAXIS) { ; Ron Lewen, 76376,2567
  z=pixel:
    z=(z^2+3z+pixel)/(z^2-3z-pixel)
      |z| <= 10
  }

RCL_13 (XAXIS) { ; Ron Lewen, 76376,2567
  z=pixel:
    z=(z^2+2z+pixel)/(z^2-2z+pixel)
      |z| <= 100
  }

RCL_14 (XAXIS) { ; Ron Lewen, 76376,2567
  z=pixel:
    z=z^pixel+pixel^z
      |z| <= 96
  }

RCL_15 (XAXIS) { ; Ron Lewen, 76376,2567
  ; Adapted from Pickover's Biomorph Zoo Collection in
  ; Figure 8.7 (p. 102).
  z=pixel:
    z=z^2.71828 + pixel,
      |real(z)| <= 100 || |imag(z)| <= 100
  }

RCL_16 (XAXIS) { ; Ron Lewen, 76376,2567
  ; Set fn1 to sqr to generate Figure 9.18 (p. 134)
  ; from Pickover's book.
  ; Set maxiter >= 1000 to see good detail in the spirals
  ; in the three large lakes.  Also set inside=0.
  z=0.5:
    z=z*pixel-pixel/fn1(z),
      abs(z) < 8
  }

RCL_Cosh (XAXIS) { ; Ron Lewen, 76376,2567
  ; Try corners=2.008874/-3.811126/-3.980167/3.779833/
  ; -3.811126/3.779833 to see Figure 9.7 (P. 123) in
  ; Pickover's Computers, Pattern, Chaos and Beauty.
  ; Figures 9.9 - 9.13 can be found by zooming.
  ; Use floating point
  z=0:
    z=cosh(z) + pixel,
      abs(z) < 40
  }

RCL_Cosh_Flip (XAXIS) { ; Ron Lewen, 76376,2567
  ; A FLIPed version of RCL_Cosh.
  ; An interesting repeating pattern with lots
  ; of detail.
  ; Use floating point
  z=0:
    z=flip(cosh(z) + pixel),
      abs(z) < 40
  }

RCL_Cosh_J { ; Ron Lewen, 76376,2567
  ; A julia-style version of RCL_Cosh above.
  ; Lots of interesting detail to zoom in on.
  ; Use floating point
  z=pixel:
    z=cosh(z) + p1,
      abs(z) < 40
  }

RCL_Cross1  { ; Ron Lewen, 76376,2567
   ; Try p1=(0,1), fn1=sin and fn2=sqr.  Set corners at
   ; -10/10/-7.5/7.5 to see a cross shape.  The larger
   ; lakes at the center of the cross have good detail
   ; to zoom in on.
   ; Use floating point.
   z=pixel:
   z=p1*fn1(fn2(z+p1)),
   |z| <= 4
   }

RCL_Cross2 { ; Ron Lewen, 76376,2567
   ; Try p1=(0,1), fn1=sin and fn2=sqr.  Set corners at
   ; -10/10/-7.5/7.5 to see a deformed cross shape.
   ; The larger lakes at the center of the cross have
   ; good detail to zoom in on.
   ; Try corner=-1.58172/.976279/-1.21088/-.756799 to see
   ; a deformed mandelbrot set.
   ; Use floating point.
   z=pixel:
   z=pixel*fn1(fn2(z+p1)),
   |z| <= 4
   }

RCL_Logistic_1 (XAXIS) { ; Ron Lewen, 76376,2567
   ; Based on logistic equation  x -> c(x)(1-x) used
   ; to model animal populations.  Try p1=(3,0.1) to
   ; see a family of spiders out for a walk <G>!
   z=pixel:
   z=p1*z*(1-z),
   |z| <= 1
   }

RCL_Mandel (XAXIS) { ; Ron Lewen, 76376,2567
   ; The traditional Mandelbrot formula with a different
   ; escape condition.  Try p1=(1,0).  This is basically the M-Set
   ; with more chaos outside.  p1=(0,0) yields a distorted M-set.
   ; Use floating point
   z=pixel:
   z=sqr(z) + pixel,
   sin(z) <= p1
   }
