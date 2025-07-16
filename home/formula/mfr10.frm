; SPDX-License-Identifier: GPL-3.0-only
;
Multifractal_10    { ;  Albrecht Niekamp  Jan2005
   ; p1 (spider)julia-seed
   ; real(p2) 5digits : (1)shape (2)outside (3)inside1 (4)inside2 (5)inside3
   ; 0_off 1_secant 2_mand 3_bees 4_jul 5_m_mods 6_phoen 7_newt 8_spider
   ;  input2: 2digits_many-mods  2digits_phoenix  2digits_spider
   ;          1digit_shapereset:0_no 1_dblmandel 2_iter-reset 3_both +5_warp
   ;          4digits reset : 0_no 1_z-reset 2_iter-reset 3_both +5_warp
   ; imag(p2)(-) 5digits_colour(bailout) number
   ;  input2 :  4digits(-)_mand/jul 2digits_secant 4digits_bees
   ; real(p3) 2digits_newt 4digits_colour1, 5digits_colour2  5digits_colour3
   ; imag(p3) shape  : factor (fn1), 5digits_colour4  6digits_colour5
   ; real(p4) outside: factor (fn2), 4+1digits_bord-out 4digits+fract_bord-in
   ; imag(p4) inside1: maxit1, 1digit_use:1_maxit 2(7)_bord-out 3(8)_bord-in
   ;                  5digits_factor1 (fn2)  4digits+fract_border1
   ; real(p5) inside2: maxit2, 1digit_use:1_maxit 2(7)_bord-out 3(8)_bord-in
   ;                  5digits_factor2 (fn3)  4digits+fract_border2
   ; imag(p5) inside3: maxit3, 1digit_use:1_maxit 2(7)_bord-out 3(8)_bord-in
   ;                  5digits_factor3 (fn4)  4digits+fract_border3
   ;  optional:  1_lake effect, 2digits_frquency 2digits_level 2digits_ampl
   ; fn(1) shared by many-mods+bees
   ;
   le=0
   z=pixel
   da=real(p2)
   dd=trunc(da)
   da=round((da-dd)*100000000000)+11111
   dd=dd+11111
   d=trunc(dd/10000)
   dd=dd-d*10000
   d3=(d==4)+(d==5)+(d==8)+(d==9)
   d4=d3==0
   vb=d>5
   sc=d==2
   mo=d==6
   px=d==7
   ab=px+(d==3)+(d==5)+(d==9)
   d=trunc(dd/1000)
   dd=dd-d*1000
   ex1=d>1
   sc1=d==2
   mo1=d==6
   px1=d==7
   v1m=mo1+px1
   v1j=d>7
   dd1=v1j+(d==4)+(d==5)
   ab1=px1+(d==3)+(d==5)+(d==9)
   d=trunc(dd/100)
   dd=dd-d*100
   ex2=d>1
   sc2=d==2
   mo2=d==6
   px2=d==7
   v2m=mo2+px2
   v2j=d>7
   dd2=v2j+(d==4)+(d==5)
   ab2=px2+(d==3)+(d==5)+(d==9)
   d=trunc(dd/10)
   ex3=d>1
   sc3=d==2
   mo3=d==6
   px3=d==7
   v3m=mo3+px3
   v3j=d>7
   dd3=v3j+(d==4)+(d==5)
   ab3=px3+(d==3)+(d==5)+(d==9)
   d=dd-d*10
   ba=imag(p5)
   mi3=trunc(ba)
   dd=(d>1)+(mi3>1)
   ex4=dd==2
   sc4=d==2
   mo4=d==6
   px4=d==7
   v4m=mo4+px4
   v4j=d>7
   dd4=v4j+(d==4)+(d==5)
   ab4=px4+(d==3)+(d==5)+(d==9)
   ;
   mm=trunc(da/1000000000)
   da=da-mm*1000000000
   ph=trunc(da/10000000)/100
   da=da-ph*1000000000
   sp=trunc(da/100000)/100
   da=da-sp*10000000
   d=trunc(da/10000)
   ex0=d>4
   da=da-d*10000
   d=d-5*ex0
   dm=(d==2)+(d==4)
   ir0=(d==3)+(d==4)
   d=trunc(da/1000)
   w1=d>4
   da=da-d*1000
   d=d-5*w1
   rs1=(d==2)+(d==4)
   ir1=(d==3)+(d==4)
   d=trunc(da/100)
   w2=d>4
   da=da-d*100
   d=d-5*w2
   rs2=(d==2)+(d==4)
   ir2=(d==3)+(d==4)
   d=trunc(da/10)
   w3=d>4
   da=da-d*10
   d=d-5*w3
   rs3=(d==2)+(d==4)
   ir3=(d==3)+(d==4)
   d=round(da)
   w4=d>4
   d=d-5*w4
   rs4=(d==2)+(d==4)
   tt=ex1+ex0+ex2+ex3+ex4
   ;
   d=real(p3)
   dd=trunc(d)
   da=(d-dd)*10000000000
   pp=trunc(dd/10000)
   ba1=dd-10000*pp
   ba2=trunc(da/100000)
   ba3=da-100000*ba2
   ;
   d=imag(p3)
   sfac=trunc(d)
   da=(d-sfac)*100000000000
   ba4=trunc(da/1000000)
   ba5=da-ba4*1000000
   ;
   d=real(p4)
   ofac=trunc(d)
   da=(d-ofac)*10000000000
   bh=trunc(da/100000)/10
   bl=(da-bh*1000000)/10
   bs=bl/2
   ;
   d=imag(p2)
   t=d<0
   if (t)
      d=-d
   endif
   dd=trunc(d)
   da=round((d-dd)*10000000000)
   d=trunc(dd/10000)
   dd=dd-d*10000
   bb0=ba1*(d==1)+ba2*(d==2)+ba3*(d==3)+ba4*(d==4)+ba5*(d==5)
   d=trunc(dd/1000)
   dd=dd-d*1000
   bb1=ba1*(d==1)+ba2*(d==2)+ba3*(d==3)+ba4*(d==4)+ba5*(d==5)
   d=trunc(dd/100)
   dd=dd-d*100
   bb2=ba1*(d==1)+ba2*(d==2)+ba3*(d==3)+ba4*(d==4)+ba5*(d==5)
   d=trunc(dd/10)
   dd=dd-d*10
   bb3=ba1*(d==1)+ba2*(d==2)+ba3*(d==3)+ba4*(d==4)+ba5*(d==5)
   d=round(dd)
   bb4=ba1*(d==1)+ba2*(d==2)+ba3*(d==3)+ba4*(d==4)+ba5*(d==5)
   ;
   d=da
   p0=trunc(d/100000000)/10
   d=d-p0*1000000000
   p6=trunc(d/1000000)/10
   d=d-p6*10000000
   if (t)
      p6=-p6
   endif
   p7=trunc(d/10000)/10
   d=d-p7*100000
   dp=p6+p0/100
   p8=trunc(d/100)/100
   d=d-p8*10000
   p9=d/100
   ;
   d=imag(p4)
   mi1=trunc(d)
   da=(d-mi1)*100000000000
   d=trunc(da/10000000000)
   bt1=d>6
   da=da-d*10000000000
   d=d-5*bt1
   dt1=d>1
   iv1=d==3
   fac1=trunc(da/100000)
   da=da-fac1*100000
   bo1=(da/100000)/10
   ;
   d=real(p5)
   mi2=trunc(d)
   da=(d-mi2)*100000000000
   d=trunc(da/10000000000)
   bt2=d>6
   da=da-d*10000000000
   d=d-5*bt2
   dt2=d>1
   iv2=d==3
   fac2=trunc(da/100000)
   da=da-fac2*100000
   bo2=(da/100000)/10
   ;
   if (mi3==1)
      d=(ba-mi3)*1000000
      fr=round((trunc(d/10000))*10)     ; lake effect by S.Gallet
      d=d-fr*1000
      lv=(trunc(d/100))/100
      d=d-lv*10000
      am=d/100
      u=real(rotskew*pi/180)
      t=exp(-flip(u))
      bo=1/real(magxmag)
      q=bo/0.75*imag(magxmag)
      dd=tan(imag(rotskew*pi/180))
      da=2*q*t
      rs=2*bo*(dd+flip(1))*t
      zz=center+(-q-bo*dd-flip(bo))*t
      z=z-zz
      d=imag(conj(da)*z)/imag(conj(da)*rs)
      le=d<=lv
      if (le)
         dd=lv-d
         z=z+2*dd*(1+am*sin(fr*dd^0.2))*rs
      endif
      z=z+zz
   else
      da=(ba-mi3)*100000000000
      d=trunc(da/10000000000)
      bt3=d>6
      da=da-d*10000000000
      d=d-5*bt3
      dt3=d>1
      iv3=d==3
      fac3=trunc(da/100000)
      da=da-fac3*100000
      bo3=(da/100000)/10
   endif
   ;
   if (vb)
      if (d3)
         if (ab)
            z=z*le+pixel*(le==0)        ; Spider
            c=p1
         else
            z=z*le+pixel*(le==0)
            c=p1                        ; newton
         endif
      elseif (ab)
         c=z                            ; Phoenix
         z=z*le+pixel*(le==0)
      else
         c=0.4*log(sqr(z^mm))           ; many mods
         z=0
      endif
   elseif (d3)
      if (ab)
         c=p1                           ; Julia
         z=z*le+pixel*(le==0)
      else                              ; bees
         c=p1
         z=z*le+pixel*(le==0)
      endif
   elseif (ab)
      c=z                               ; Mandel
      z=0
   else
      c=z                               ; Secant
      z=z*le+pixel*(le==0)
   endif
   t=0
   bo=|z|
   p=pp
   z0=p7
   zold=(0.0,0.0)
   cb=p9
   ba=bb0
   :
   if (tt)
      t=t+1
      if (ex0)
         ex0=t<mi1
         if (bo>bs)
            u=2*(fn1(t/sfac))
            ex0=0
            if (ir0)
               t=0
            endif
            if (d4)
               z=z*u
               if (mo)
                  c=0.4*log(sqr(z^mm))
               else
                  c=pixel
               endif
            else
               z=z*le+pixel*(le==0)
               cb=p9*u
               c=p1*u
               p=pp*u
            endif
            tt=tt-1+ex0
         endif
      elseif ((ex1)&&bo>bl)
         if (bo<bh)
            d3=dd1
            ba=bb1
            ab=ab1
            ex1=0
            tt=tt-1
            if (w1)
               u=2*(fn2(t/ofac))
            else
               u=1,0
            endif
            if (ir1)
               t=0
            endif
            if (d3)
               vb=v1j
               if (rs1)
                  z=pixel
                  cb=p9*u
                  c=p1*u
                  p=pp*u
               else
                  c=p1
                  z=z*u
                  cb=p9
               endif
            else
               vb=v1m
               if (rs1)
                  c=z*u
                  z=pixel*(sc1+px1)
                  z0=p7*u
                  ph=ph*u
               else
                  c=z
                  z=z*u
               endif
               if (mo1)
                  c=0.4*log(sqr(z^mm))
               endif
            endif
         endif
      elseif (ex2)
         if (dt1)
            if (iv1)
               d=bo>bo1
            else
               d=bo<bo1
            endif
            if (bt1)
               d=d+(t>mi1)
            endif
         else
            d=t>mi1
         endif
         if (d)
            ab=ab2
            d3=dd2
            ba=bb2
            ex2=0
            tt=tt-1
            if (w2)
               u=2*(fn2(t/fac1))
            else
               u=1,0
            endif
            if (ir2)
               t=0
            endif
            if (d3)
               vb=v2j
               if (rs2)
                  z=pixel
                  cb=p9*u
                  c=p1*u
                  p=pp*u
               else
                  cb=p9
                  c=p1
                  z=z*u
               endif
            else
               vb=v2m
               if (rs2)
                  c=z*u
                  z=pixel*(sc2+px2)
                  z0=p7*u
                  ph=ph*u
               else
                  c=z
                  z=z*u
               endif
               if (mo2)
                  c=0.4*log(sqr(z^mm))
               endif
            endif
         endif
      elseif (ex3)
         if (dt2)
            if (iv2)
               d=bo>bo2
            else
               d=bo<bo2
            endif
            if (bt2)
               d=d+(t>mi2)
            endif
         else
            d=t>mi2
         endif
         if (d)
            ab=ab3
            d3=dd3
            ba=bb3
            ex3=0
            tt=tt-1
            if (w3)
               u=2*(fn3(t/fac2))
            else
               u=1,0
            endif
            if (ir3)
               t=0
            endif
            if (d3)
               vb=v3j
               if (rs3)
                  z=pixel
                  cb=p9*u
                  c=p1*u
                  p=pp*u
               else
                  cb=p9
                  c=p1
                  z=z*u
               endif
            else
               vb=v3m
               if (rs3)
                  c=z*u
                  z=pixel*(sc3+px3)
                  z0=p7*u
                  ph=ph*u
               else
                  c=z
                  z=z*u
               endif
               vb=v3m
               if (mo3)
                  c=0.4*log(sqr(z^mm))
               endif
            endif
         endif
      elseif (ex4)
         if (dt3)
            if (iv3)
               d=bo>bo3
            else
               d=bo<bo3
            endif
            if (bt3)
               d=d+(t>mi3)
            endif
         else
            d=t>mi3
         endif
         if (d)
            ab=ab4
            d3=dd4
            ba=bb4
            ex4=0
            tt=0
            if (w4)
               u=2*(fn4(t/fac3))
            else
               u=1,0
            endif
            if (d3)
               vb=v4j
               if (rs4)
                  z=pixel
                  cb=p9*u
                  c=p1*u
                  p=pp*u
               else
                  cb=p9
                  c=p1
                  z=z*u
               endif
            else
               vb=v4m
               if (rs4)
                  c=z*u
                  z=pixel*(sc4+px4)
                  z0=p7*u
                  ph=ph*u
               else
                  c=z
                  z=z*u
               endif
               if (mo4)
                  c=0.4*log(sqr(z^mm))
               endif
            endif
         endif
      endif
   endif
   if (vb)
      if (d3)
         if (ab)
            z=z*z+c                     ; Spiderjul     John Horner
            c=c*sp+z
         else
            z1=z^p-1                    ; Qusinewton    Pusk s Istv n
            z2=p*z*z
            z=z-z1/z2
         endif
      elseif (ab)
         z1=z*z+0.56+ph/100-0.5*zold    ; Phoenix       Mike Wareman
         zold=z
         z=z1
      else
         z2=fn1(z)+c                    ; Many_mods     Linda Allison
         z1=cos(z2)
         z=c*(1-z1)/(1+z1)
      endif
   elseif (d3)
      if (ab)
         z2=z*z                         ; Julia         Pusk s Istv n
         z=z2*z2+p6*z2+c-p0
      else
         z1=fn1(z)-cb                   ; Bees          Ray Girvan
         z2=z1^p8-1
         z3=p8*(z1^(p8-1))
         z=z-(z2/z3)
      endif
   elseif (ab)
      if (dm)
         z=z*z+c+c*c-dp                 ; Double Mandel
      else
         z2=z*z                         ; Mandel        Pusk s Istv n
         z=z2*z2+p6*z2+c-p0
      endif
   else
      z3=z                              ; Secant        Mike Wareman
      z1=z0*z0*z0*z0-1
      z2=z*z*z*z-1
      z=z-z2*(z-z0)/(z2-z1)
      z0=z3
   endif
   bo=|z|
   bo<ba
}
