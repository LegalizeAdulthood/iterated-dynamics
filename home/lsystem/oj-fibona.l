; All the systems in this file is done
;  by Ole Jorgensen (blwl@sol.dk)
;

Boat { ; Try order 20
  Angle 4
  Axiom f
  f=+f-f
}

Boat2 { ; Try order 17
  Angle 4
  Axiom f
  f=+ff
}

Boat3 { ;
  Angle 6
  Axiom f
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=+FFFFFFFF+FFFFF+FFF+FF+F|++++
}

Boat4 { ; 180 * .75 = 135 -->
  Angle 4
  Axiom F
  F=D\180D\135D\101.25D\75.9375D\56.953125D\42.71484375D
  F=\32.0361328125D\24.027099609375D\18.02032470703125D\13.5152435302734375D
  F=+FF
}

Boat5 { ;
  Angle 3
  Axiom -F+F-F+F-F+F
  F=-F+F-FF+FFF
}

Boat6 { ; 
  Angle 3
  axiom F
  F=-F<25+K 
  K=+F<25+F<25+L
  M=-F<25-F<25-F<25-F<25-F<25-F<25-F<25+N
  N=+FC1+F<21+F<21F<21+++F<21F<21F<21+++++F<21F<21F<21F<21F<21
}

; the boat was number 1
;
; The names
; ---------
;     1. combination is +F-F+FF-FFF....
; b  :2. combinatuon is +F[f]-F[f]...+x[f]
; b2 :As b but first F[f] is missing.
; c  :4. combination is +F[t]....-x[t] where t = +F-F+FFF....-x
;     T is also used with comb. Either as +F-F+FFF... or as ...+FFF-FF+F-F
; +  :Plus over the place.
; -  :mostly - 
; ang:No 'ang' in the name: the angle is 90 degrees
; ax :Ax4 = Axiom +F+F+F+F 
;comb:combined:   +FFFFF[t]....-F[ttttt]
;de+-:descending: +F-F+FF-FFF....-FF+F-F
;de+ :  -     -      -     : +F+F+FF+FFF....-FF-F-F
; sp :spiral. As in 1. combination, but first +F is missing.
; M  :First +/- missing.
;     

02 { ; 
  Angle 4
  Axiom F
  F=+F-F+FF
}

03 { ; 
  Angle 4
  Axiom F
  F=+F-F+FF-FFF
}

03Ang3ax3 { ;
  Angle 3
  Axiom F+F+F
  F=+F-F+FF-FFF
}

05 { ;
  Angle 4
  Axiom F
  F=+F-F+FF-FFF+FFFFF
}

08 { ;
  Angle 4
  Axiom F
  F=+F-F+FF-FFF+FFFFF-FFFFFFFF
}

13 { ; 
  Angle 4
  Axiom F
  F=+F-FF+FFF-FFFFF+FFFFFFFF-FFFFFFFFFFFFF
}

13Ax4 { ;
  Angle 4
  Axiom F+F+F+F
  F=+F-F+FF-FFF+FFFFF-FFFFFFFF+FFFFFFFFFFFFF
}

21 { ; 
  Angle 4
  Axiom F
  F=+F-FF+FFF-FFFFF+FFFFFFFF-FFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFF
}

01b { ; 
  Angle 4
  Axiom F
  f=-F[+f]+F[-f]
}

02b { ; 
  Angle 4
  Axiom F
  f=-F[+f]+F[-f]-FF[+f]
}

03b { ; 
  Angle 4
  Axiom F+f+f+f
  f=-F[+f]+F[-f]-FF[+f]+FFF[-f]
}

05b { ; 
  Angle 4
  Axiom F
  f=-F[+f]+F[-f]-FF[+f]+FFF[-f]-FFFFF[+f]
}

08b { ; 
  Angle 4
  Axiom F
  f=+F[-f]-FF[+f]+FFF[-f]+FFFFF[+f]+FFFFFFFF
}

03b2 { ; 
  Angle 4
  Axiom F+f+f+f
  f=+F[-f]-FF[+f]+FFF[-f]
}

02b2 { ; 
  Angle 4
  Axiom F
  f=+F[-f]-FF[+f]
}

02c { ; 
  Angle 4
  Axiom F+F+F
  F=+F[t]-F[t]+FF[t]
  t=+F-F+FF
}

03c { ; 
  Angle 4
  Axiom F
  F=+F[t]-F[t]+FF[t]-FFF[t]
  t=+F-F+FF-FFF
}

05c { ;
  Angle 4
  Axiom F
  F=+F[t]-F[t]+FF[t]-FFF[t]+FFFFF[t]
  t=+F-F+FF-FFF+FFFFF
}

08cAng3 {  ;
  Angle 3
  Axiom F
  F=+F[t]-F[t]+FF[t]-FFF[t]+FFFFF[t]-FFFFFFFF[t]
  t=-F+F-FF+FFF-FFFFF+FFFFFFFF
}

13c {  ;
  Angle 4
  Axiom F
  F=+F[t]-F[t]+FF[t]-FFF[t]+FFFFF[t]-FFFFFFFF[t]+FFFFFFFFFFFFF[t]
  t=+F-F+FF-FFF+FFFFF-FFFFFFFF+FFFFFFFFFFFFF
}

02c- { ; 
  Angle 4
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]
  t=-F-F-FF
}

02c-Ang6 {  ;
  Angle 6
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]
  t=-F-F-FF
}

03c-Ang3 { ;  
  Angle 3
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]
  t=-F-F-FF-FFF
}

03c- { ; 
  Angle 4
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]
  t=-F-F-FF-FFF
}

05c-Ang3 { ; 
  Angle 3
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]
  t=-F-F-FF-FFF-FFFFF
}

05c- { ;
  Angle 4
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]
  t=-F-F-FF-FFF-FFFFF
}

08c-Ang3 {  ;
  Angle 3
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]-FFFFFFFF[t]
  t=-F-F-FF-FFF-FFFFF-FFFFFFFF
}

08c- {  ;
  Angle 4
  Axiom F+F+F
  F=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]-FFFFFFFF[t]
  t=-F-F-FF-FFF-FFFFF-FFFFFFFF
}

13c- { ; 
  Angle 4
  Axiom F
  F=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]-FFFFFFFF[t]-FFFFFFFFFFFFF[t]
  t=-F-F-FF-FFF-FFFFF-FFFFFFFF-FFFFFFFFFFFFF
}

03+Ang3 {  ;
  Angle 3
  Axiom f+f+f
  F=+F+F+FF+FFF
}

03+Ang4 {  ;
  Angle 4
  Axiom F
  F=+F+F+FF+FFF
}

03+Ang6 {  ;
  Angle 6
  Axiom F
  F=+F+F+FF+FFF
}

05+Ang3 {  ;
  Angle 3
  Axiom F
  F=+F+F+FFF+FFF+FFFFF
}

05+ { ;
  Angle 4
  Axiom F
  F=+F+F+FF+FFF+FFFFF
}

05+Ang6Color {  ;
  Angle 6
  Axiom F
  F=+C1F+<64F+<64FF+<64FFF+<64FFFFF;F
}

05+Ang6Ax3 {  ;
  Angle 6
  Axiom F+F+F
  F=+F+F+FF+FFF+FFFFF
}

08+ { ;
  Angle 4
  Axiom c1F+c2F+c3F+c4F
  F=+F+F+FF+FFF+FFFFF+FFFFFFFF
}

08sp {  ;
  Angle 4
  Axiom F
  F=-F+F+FF+FFF+FFFFF+FFFFFFFF
}

08+Ax4Color { ; 
  Angle 4
  Axiom c10F+c20F+c30F+c40F
  F=+F+F+FF+FFF+FFFFF+FFFFFFFF
}

21+Ang3Color { ;
  Angle 3
  Axiom f
  f=+Fc64+Fc32+FFc64+FFFc32+FFFFFc64+FFFFFFFFc32
  f=+FFFFFFFFFFFFFc64+FFFFFFFFFFFFFFFFFFFFFc32
}

21+ {  ;
  Angle 4
  Axiom f
  f=+F+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFF
}

03M+ { ; 
  Angle 4
  Axiom F
  F=F+F+FF+FFF
}

03M_Ax4 { ; 
  Angle 4
  Axiom F+F+F+F
  F=F-F+FF-FFF
}

03M+_Ang3 { ; 
  Angle 3
  Axiom F
  F=F+F+FF+FFF
}

03M+_Ang6 {
  Angle 6
  Axiom F
  F=F+F+FF+FFF
}

03M+_Ang6Col { ; 
  Angle 6
  Axiom F
  F=c1F+c36F+c72Fc108F+c144Fc180Fc216F
}

03M+_Col{ ; 
  Angle 4
  Axiom F
  F=<1F+<1F+FF>1+FFF>1
} 

03M+_Ang6.2 {
  Angle 6
  Axiom F
  F=<1F+<1F+FF>1+FFF>1
}

08M+ { ;
  Angle 4
  Axiom F
  F=F+F+FF+FFF+FFFFF+FFFFFFFF
}

08M_Ang3 { ; 
  Angle 3
  Axiom F
  F=F+F-FF+FFF-FFFFF+FFFFFFFF
}

08M+Ang6 { ;
  Angle 6
  Axiom F
  F=F+F+FF+FFF+FFFFF+FFFFFFFF
}

08MColor { ; 
  Angle 6
  Axiom F
  F=c1F+c4F-c7FF+c12FFF-c20FFFFF+c33FFFFFFFF
}

08M_Ang6 { ;
  Angle 6
  Axiom F
  F=F+F+FF-FFF-FFFFF-FFFFFFFF
}

21M- {  ;
  Angle 4
  Axiom f
  f=F-F-FF-FFF-FFFFF-FFFFFFFF-FFFFFFFFFFFFF-FFFFFFFFFFFFFFFFFFFFF
}

sp02-Ang3 { ; 
  Angle 3
  Axiom F
  F=-F-FF
}

sp02-Ang6 { ; 
  Angle 6
  Axiom F
  F=-F-FF
}

sp08+ { ; 
  Angle 4
  Axiom F
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
}

sp13+ { ; 
  Angle 4
  Axiom F
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
}

sp13-Ax4 { ;
  Angle 4
  Axiom F+F+F+F
  F=-F-FF-FFF-FFFFF-FFFFFFFF-FFFFFFFFFFFFF
}

Comb05Ang3 { ;
  Angle 3
  Axiom F
  F=-F[T]+F[T]-FF[TT]+FFF[TTT]-FFFFF[TTTTT]
  T=-FFFFF+FFF-FF+F-F
}

Comb05_2Ang3  { ;
  Angle 3
  Axiom F
  F=-F[ttttt]+F[ttt]-FF[tt]+FFF[t]-FFFFF[t]
  T=+F-F+FF-FFF+FFFFF
}

Comb08-Ax3{  ;
  Angle 3
  Axiom f+f+f
  f=-F[t]-F[t]-FF[t]-FFF[t]-FFFFF[t]-FFFFFFFF[t]
  t=-F-F-FF-FFF-FFFFF-FFFFFFFF
}

de05+Ang3 { ; 
  Angle 3
  Axiom F
  F=+F+F+FF+FFF+FFFFF
  F=-FFF-FF-F-F
}

de08+Ang3 { ; 
  Angle 3
  Axiom F
  F=+F+F+FF+FFF+FFFFF+FFFFFFFF
  F=-FFFFF-FFF-FF-F-F
}

de08+-Ang6 { ;
  Angle 6
  Axiom F
  F=F+F-FF+FFF-FFFFF+FFFFFFFF+FFFFF-FFF+FF-F+F
}

de13+Ang3 { ; 
  Angle 3
  Axiom F
  F=+F+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F-F
}

de13+-Ang3 { ; 
  Angle 3
  Axiom F
  F=+F-F+FF-FFF+FFFFF-FFFFFFFF+FFFFFFFFFFFFF
  F=+FFFFFFFF-FFFFF+FFF-FF+F-F
}

de13+- { ; 
  Angle 4
  Axiom F
  F=+F-F+FF-FFF+FFFFF-FFFFFFFF+FFFFFFFFFFFFF
  F=+FFFFFFFF-FFFFF+FFF-FF+F-F
}

de21+-Ang3 {
   Angle 3
   Axiom F
   F=+F+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFF
   F=-FFFFFFFFFFFFF-FFFFFFFF-FFFFF-FFF-FF-F-F
}

de34+Ang3 { ; 
  Angle 3
  Axiom F
  F=+F+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=-FFFFFFFFFFFFFFFFFFFFF-FFFFFFFFFFFFF-FFFFFFFF-FFFFF-FFF-FF-F-F
}

de34+-Ang3 { ; Prøv tredje orden
  Angle 3
  Axiom F
  F=+F-F+FF-FFF+FFFFF-FFFFFFFF+FFFFFFFFFFFFF-FFFFFFFFFFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=+FFFFFFFFFFFFFFFFFFFFF-FFFFFFFFFFFFF+FFFFFFFF-FFFFF+FFF-FF+F-F
}

de_sp03+Ang30 { ; 
  Angle 30
  Axiom F
  F=+F+FF+FFF+FFFFF+FFFFFFFF
  F=-FFFFF-FFF-FF-F
}

de_sp05- { ; 
  Angle 4
  Axiom F
  F=-@0.75F-FF-FFF-FFFFF-FFF-FF-F-gggg
}

de_sp05+Ang4 { ; Same as SpiralTile05 Angle 4 
  Angle 4
  Axiom F
  F=F+F+FF+FFF+FFFFF
  F=-FFF-FF-F-F
}

de_sp08- { ; 
  Angle 4
  Axiom -F
  F=-F-FF-FFF-FFFFF-FFFFFFFF-FFFFF-FFF-FF-F
}

de_sp13- { ; 
  Angle 4
  Axiom -F
  F=-F-FF-FFF-FFFFF-FFFFFFFF-FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F
}

Islands {  ;
  Angle 3
  Axiom F-F-F-F-F-F
  F=-F+F-FF+FFF
}

R.A. {  ;
  Angle 3
  Axiom f+f+f+f
  f=+F-[-v]-FF++[+b]+FFF-[-n]-FFFFF+[+m]+FFFFFFFFF
  v=-F
  b=-F--F
  n=-F--F---F
  m=-F--F---F-----F
}

R.A.2 {  ;
  Angle 3
  Axiom f
  f=+FF-[-v]-FFF
  v=-F
  b=-F--F
  n=-F--F---F
  m=-F--F---F-----F
}

Snow {  ;
  Angle 4
  Axiom F+F+F+F+F
  F=-FF-FFFF-FFFF-FFFFFFFF
} 

Snow2 { ;
  Angle 4
  axiom F+F+F+F
  F=+F+H 
  H=-F-F-F-J
  J=+F+F+F+F+F+K
  K=-F-F--FF---FFF-FFFFF
}

Snow3 { ;
  Angle 4
  axiom F+F+F+F
  F=-F+K 
  K=+F+F+F+L
  M=-F-F-F-F-F-F-F+N
  N=+F+F+FF+++FFF+FFFFF
}

Snow4{   ;     
  Angle 4
  axiom F+F+F+F
  F=+F+K 
  K=-F-L
  M=+F+N
  N=-F-F--FF---FFF-FFFFF
}

String21 { ; Prøv froth616.map
  Angle 10
  Axiom f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f
  f=<1\1d\1d\2d\3d\5d\8d\13d\21d\13d\8d\5d\3d\2d\1d\1d
}

String33 { ; Prøv froth616.map
  Angle 10
  Axiom +f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f
  f=<1\1d\1d\2d\3d\5d\8d\13d\21d\34d\21d\13d\8d\5d\3d\2d\1d\1d
}

String55 { ; 
  Angle 10
  Axiom f+f+f+f+f+f+f+f+f+f+f+f+f+f
  f=\1d\1d\2d\3d\5d\8d\13d\21d\34d\55d\34d\21d\13d\8d\5d\3d\2d\1d\1d
}

String55Color { ; Prøv froth616.map
  Angle 10
  Axiom f+f+f+f+f+f+f+f+f+f+f+f+f+f
  f=<1\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
}

String55_2Col { ; Prøv froth616.map
  Angle 10
  Axiom f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f+f
  f=<1\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
}

Moebius { ; 
  Angle 10
  Axiom f
  f=f[x]
  x=ff[stuv]
  s=\1d\1d\2d\3d\5d\8d\13d\21d\34d\55d\34d\21d\13d\8d\5d\3d\2d\1d\1d
  t=!\1d\1d\2d\3d\5d\8d\13d\21d\34d\55d\34d\21d\13d\8d\5d\3d\2d\1d\1d
  u=\1d\1d\2d\3d\5d\8d\13d\21d\34d\55d\34d\21d\13d\8d\5d\3d\2d\1d\1d
  v=!\1d\1d\2d\3d\5d\8d\13d\21d\34d\55d\34d\21d\13d\8d\5d\3d\2d\1d\1d
}

MoebiusColor { ; Prøv Froth616.maps 
  Angle 10
  Axiom f
  f=f[x]
  x=ff[stuv]
  s=<1\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
  t=<1!\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
  u=<1\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
  v=<1!\1d<1\1d<1\2d<1\3d<1\5d<1\8d<1\13d<1\21d<1\34d<1\55d<1\34d<1\21d<1\13d<1\8d<1\5d<1\3d<1\2d<1\1d<1\1d
}

SpiralSinSpirals3 { ;
  Angle 4
  Axiom +F+FF+FFF
  F=+S
  S=+F+H
  H=+F+F+F
}

SpiralsInSpirals5 { ;
  Angle 4
  Axiom +F+FF+FFF+FFFFF
  F=+S
  S=+F+H
  H=+F+F+I
  I=+F+F+F+F+J
  J=+F+F+F+F+F+F+F+F
}

SpiralsInSpirals8 { ;
  Angle 4
  Axiom +F+F+FF+FFF+FFFFF+FFFFFFFF
  F=+S
  S=+F+H
  H=+F+F+I
  I=+F+F+F+F+J
  J=+F+F+F+F+F+F+F+F
}

SpiralTile02 { ;
  Angle 6
  Axiom F+F+F+F+F+F
  F=+<3F+<3FF>3->3F
}

SpiralTile03 { ;
  Angle 6
  Axiom F+F+F
  F=+c1F+c2FF+c3FFF-c2FF-c1F
}

SpiralTile05 { ;
  Angle 6
  Axiom f+f+f 
  F=+c1F+c2FF+c3FFF+c4FFFFF
  F=-c3FFF-c2FF-c1F
}

SpiralTile13{ ;
  Angle 6
  Axiom c10f+c20f+c30f
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F
}

SpiralTile21Ang3{ ;
  Angle 3
  Axiom f;-f-f   ;Fjern markeringen hvis 1.order skal være en flise
  F=+c1F+c2FF+c3FFF+c4FFFFF+c5FFFFFFFF+c6FFFFFFFFFFFFF+c7FFFFFFFFFFFFFFFFFFFFF
  F=-c6FFFFFFFFFFFFF-c5FFFFFFFF-c4FFFFF-c3FFF-c2FF-c1F
}

SpiralTile55{ ;
  Angle 6
  Axiom f;+f+f
  F=+c1F+c2FF+c3FFF+c4FFFFF+c5FFFFFFFF+c6FFFFFFFFFFFFF
  F=+c7FFFFFFFFFFFFFFFFFFFFF+c8FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=+c9FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF 
  F=-c8FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF-c7FFFFFFFFFFFFFFFFFFFFF
  F=-c6FFFFFFFFFFFFF-c5FFFFFFFF-c4FFFFF-c3FFF-c2FF-c1F
}

SpiralCurve03 { ; 
  Angle 6
  Axiom +F+F+FF+FFF-FF-F-F
  f=+f+f+ff+fff-ff-f-f
}

SpiralCurve03_2 { ; 
  Angle 6
  Axiom -C52f>10fC32f>10fC11f-C52fC32fC11f-C52fC11f-C52f-C32f+C11f+C52f+C32fC11f+C52fC32fC11f+C52f>10fC32f>10fC11f
  f=+f+f+ff+fff-ff-f-f
}

SpiralCurve13 { ;
  Angle 6
  Axiom f
  F=+@0.991<1F+<1FF+<1FFF+<1FFFFF+<1FFFFFFFF+<1FFFFFFFFFFFFF
  F=-<1FFFFFFFF-<1FFFFF-<1FFF-<1FF-<1F|----
}

SpiralCurve55 { ;
  Angle 6
  Axiom f
  F=+@.85F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=+FFFFFFFFFFFFFFFFFFFFF+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=+FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=-FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF-FFFFFFFFFFFFFFFFFFFFF
  F=-FFFFFFFFFFFFF-FFFFFFFF-FFFFF-FFF-FF-F|----
}

SpiralCurve55Col{ ;
  Angle 6
  Axiom f
  F=+@.85<1F+<1FF+<1FFF+<1FFFFF+<1FFFFFFFF+<1FFFFFFFFFFFFF
  F=+<1FFFFFFFFFFFFFFFFFFFFF+<1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=+<1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
  F=-<1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF-<1FFFFFFFFFFFFFFFFFFFFF
  F=-<1FFFFFFFFFFFFF-<1FFFFFFFF-<1FFFFF-<1FFF-<1FF-<1F|----
}

SpiralCurve13_2{ ;
  Angle 6
  Axiom f+f+f+f+f+f
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F|----
}

SpiralCurve13_2B{ ;
  Angle 6
  Axiom f+f+f;
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F|-
}

SpiralCurve13_2C{ ;
  Angle 6
  Axiom f+f+f+f+f+f+f+f-f-f-f-f-f-f-f-f
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F|----
}

SpiralCurve13_2D{ ;
  Angle 6
  Axiom f+f+f+f-f-f-f-f-f-f-f-f+f+f+f+f+f+f+f+f-f-f-f-f-f-f-f-f+f+f+f+f
  F=+F+FF+FFF+FFFFF+FFFFFFFF+FFFFFFFFFFFFF
  F=-FFFFFFFF-FFFFF-FFF-FF-F|----
}

SpiralKurve08 { ;
  Angle 6
  Axiom f+f+f+f-f-f-f-f+f+f+f+f-f-f-f-f
  F=+F+FF+FFF+FFFFF+FFFFFFFF
  F=-FFFFF-FFF-FF-F|----
}

Curve5{ ;
  Angle 4
  axiom F+F+F+F
  F=+F+K 
  K=-F-F-L
  M=+F+F+F+N
  N=-F-F--FF---FFF-FFFFF
}

CloudOutline { ;
  Angle 4
  Axiom f
  f=-F[+t]+F[-t]-FF[++t]+FFF[---t]
  t=-F+F 
}

Escher { ; 
  Angle 6
  Axiom flpwe
  f=-c1F[l]-<51F[l]-<51FF[l]-<51FFF[l];-FFFFF[l]
  l=-<51F[p]<51F[p]<51FF[p];-FFF[p]
  p=-<51F[w]-<51F[w];-FF[w]
  w=-<51F[e];-c30F[e]
  e=F
}

Flower { ; 
  Angle 30
  Axiom F
  F=+a-b-kk-eee-hhhhh-iiiiiiii-jjjjjjjjjjjjj
  a=+b-k-ee-hhh-iiiii-jjjjjjjj
  b=+k-e-hh-iii-jjjjj
  k=+e-h-ii-jjj
  e=+h-i-jj
  h=+i-j
  i=+j
  j=F
}

Flower01 { ; 
  Angle 15
  Axiom F+F+F
  F=<2H-<2E-<2rr-<2lll>2-wwwww>2-tttttttt>2-SSSSSSSSSSSSS>2-
  h=<3E-<3r-<3ll-www>3-ttttt>3-SSSSSSSS>3-
  e=<4r-l-<4ww-ttt>4-SSSSS>4-
  r=<5l-<5w-tt>5-SSS>5-
  l=<6w-<6t>6-SS>6-
  w=<7t-S>7-
  t=<8s>8-
  s=F-
}

Flower02 { ;
  Angle 50
  Axiom F
  F=+aaaaaaaaaaaaa-bbbbbbbb+lllll-hhh+ii-j+k
  a=-bbbbbbbb+lllll-hhh+ii-j+k
  b=+lllll-hhh+ii-j+k
  l=-hhh+ii-j+k
  h=+ii-j+k
  i=-j+k
  j=+k
  k=-F
}

Flower03 {; 
  Angle 40
  Axiom F
  F=+<1aaaaaaaaaaaaa-bbbbbbbb+lllll-hhh+ii-j+k>1
  a=-<2b+b-b+b-b+b-b+b+l-l+l-l+l-h-h+h+i-i-j+k>2
  b=+<3l-l+l-l+l-h-h+h+i-i-j+k>3
  l=-<4h-h+h+i-i-j+k>4
  h=+<5i-i-j+k>5
  i=-<6j+k>6
  j=+<6k>6
  k=-F
}

Flower04 { ; 
  Angle 50
  Axiom F
  F=-<5a-<5b-<5ll-hhh>5-iiiii>5-jjjjjjjj>5
  a=-<4b-<4l-<4hh>4-iii>4-jjjjj>4
  b=-<3l-<3h-ii>3-jjj>3
  l=-<2h-<2i>2-jj>2
  h=-<1i-j>1
  i=-<1j>1
  j=-f
}

Flower05 { ; 
  Angle 30
  Axiom F
  F=+aaaaaaaaaaaaa-bbbbbbbb+hhhhh-iii+jj-k+l
  a=-b+b+b+b+b+b+b+b+h-h-h-h-h-i+i+i+j-j-k+l
  b=+h-h-h-h-h-i+i+i+j-j-k+l
  h=-i+i+i+j-j-k+l
  i=+j-j-k+l
  j=-k+l
  k=+l
  l=-Fabbhhhiiiiijjjjjjjj
}

Flower06 { ; 
  Angle 16
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+<1H<1E<1WA>1R>1E>1S
  h=-<1E<1W<1A>1R>1E>1S
  e=+<1W<1AR>1E>1S
  w=-<1A<1R>1E>1S
  a=+<1RE>1S
  r=-<1E>1S
  e=+<1S>1
  s=-FFFFFFFFFFFFFFFF
}

Flower07 { ; 
  Angle 8
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+<1H+<1E+<1W+>1A+>1R+>1E
  H=-<1E-<1W-A->1R->1E
  E=+<1W+<1A+>1R+>1E
  W=-<1A-R->1E
  A=+<1R+>1E
  R=-<1E>1
  E=+FFFF+FFFF
}

Flower08 { ;
  Angle 16
  Axiom ---F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+<10H+<10E+W+>10A+>10R
  H=-<20E-<20W->20A->20R
  E=+<30W+A+>30R
  W=-<40A->40R
  A=+<50R>50
  R=FFFF+FFFF
}

Flower09 { ; Prøv 4. orden
  Angle 16
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F  
  F=+<10e-<10e+<10e-<10e+<10e-<10e+<10e-<10e-<10h+<10h
  F=->10h+>10h->10h+>10a+>10a->10a->10w+>10w+>10s->10r
  e=-<20h+<20h-<20h+<20h-<20h+<20a+>20a->20a->20w+>20w+>20s->20r
  h=+<30a+<30a-<30a-w+>30w+>30s->30r
  a=-<40w+<40w+>40s->40r
  w=+<10s->10r
  s=-<10r>10
  r=+F-F+FF-FFF+FFFFF
}


Flower10 { ; 
  Angle 16 ; 32
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+<10e-e-e-e-e-e-e-e-h+h+h+h+h+a-a-a-w+w+s->10r
  e=-<20h+h+h+h+h+a-a-a-w+w-s+>20r
  h=+<30a-a-a-w+w-s+>30r
  a=-<40w+w-s+>40r
  w=+<50s+>50r
  s=-<60r>60
  r=-F-FFFFF-F-FFF-FF+FF-FFF-F-FFFFF-F
}

Flower11 { ; 
  Angle 16
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+<10e-e-e-e-e-e-e-e-h+h+h+h+h+a-a-a-w+w+s->10r
  e=-<20h+h+h+h+h+a-a-a-w+w-s+>20r
  h=+<30a-a-a-w+w-s+>30r
  a=-<40w+w-s+>40r
  w=+<50s+>50r
  s=-<60r>60
  r=-F-FFFFF-F-FFF-FF-FF-FFF-F-FFFFF-F
}

Flower12 {; 
  Angle 6
  Axiom F+F+F+F+F+F+F+F+F+F+F+F+F+F+F+F
  F=+C10e-e+e-e+e-e+e-e-h+h-h+h-h+a+a-a-w+w+s->10r
  e=-<20h+h-h+h-h+a+a-a-w+w+s->20r
  h=+<30a+a-a-w+w+s->30r
  a=-<40w+w+s->40r
  w=+<50s->50r
  s=-<60r>60
  r=+F++FFFFF+++F+++++FFF++++++++FF+++++FF+++FFF++F+FFFFF+F
}

Gitter { ; Prøv order 10000!
  Angle 6
  Axiom F+F+FF+FFF+FFFFF+FFFFFFFF
  F=<10e-e-e-e-e-e-e-e-h-h-h-h-h-a-a-a-w-w-s->10r
  e=<20h-h-h-h-h-a-a-a-w-w-s->20r
  h=<30a-a-a-w-w-s->30r
  a=<40w-w-s->40r
  w=<50s->50r
  s=<60r>60
  r=f
}

