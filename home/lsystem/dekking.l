; Dekking's L-systems
; All translation errors due to William McWorter

Dekking4M {
  Angle 8
  Axiom p
  p=+f-p+f-w-f+t
  q=---f+++u---f+++r---f+++u
  r=---f+++r---f+++u+++f---v
  s=+f-w+f-p+f-w
  t=+f-p+f-w+f-p
  u=+f-w+f-p+++f---q
  v=---f+++r---f+++u---f+++r
  w=---f+++u---f+++r-f+s
  f=
}

Dekking4 { ; Every even order is like Peano's curve
  Angle 4
  Axiom p
  p=fp+f-qfp
  q=-f+s--f++r-f+s
  r=--f++r-f+s--f++r
  s=+f-qfp+f-q
  f=
}

Dekking3 {
  Angle 7
  Axiom g+g+g+g+g+g+g
  g=[@.25f]g+[@.25f]g---[@.25f]g---[@.25f]g--
  f=
}

Dekking3err {
  Angle 7
  Axiom x+x+x+x+x+x+x
  x=[@.25c40f]gx+[@.25c44f]gx---[@.25c48f]gx++++[@.25c52f]gx
  f=
  g=
}

Dekking1Improved {
  Angle 3
  Axiom g+g+g
  f=g+f-g
  g=f+g-f
}

Dekking1err {
  Angle 3
  Axiom f+f+f
  f=g+f-g
  g=f+g-g
}

Dekking5 { ; Dekking's boundary of the Twindragon
  Angle 4
  Axiom otuz
  f=
  o=fo+f-t
  p=++f--u+f-x
  q=-f+v++f--q
  r=-f+zfs
  s=fw
  t=++f--u
  u=++f--y
  v=fs
  w=fo+f-p
  x=++f--y+f-x
  y=-f+r++f--q
  z=-f+zfw
}

Dekking { ; Dekking's Church 
  ; Advances in Math, vol. 44, 1982, pp. 78-104
  Angle 4
  Axiom C32WC06ZC15YC06X
  F=
  W=FW+F-XFW-F+Z
  X=++F--Y-F+Z++F--Y-F+Z
  Y=++F--Y+F-X
  Z=FW+F-X
}

Dekking8 {
  Angle 4
  Axiom z
  z=x
  x=fxfx+fy+fy-fx-fx+fy+fyfx-fy-fxfxfy+fx-fy-fxfx-fy+fxfy+fy+fx-fx-fyfy+
  y=-fxfx+fy+fy-fx-fxfy-fx+fyfy+fx+fy-fxfyfy+fx+fyfx-fx-fy+fy+fx-fx-fyfy
  f=
}

Dekking7 { ; needs fixing
  Angle 4
  Axiom x
  x=fx+fvfv-yfx-fvfv+wfx-fvfv+wfx+fvfv-y
  y=++f--z+fvfv-y++f--z+fvfv-yfx-fvfv+wfx+fvfv-y
  z=++f--z+fvfv-y++f--z+fvfv-y++f--z-fvfv+wfx-fvfv+w
  z=++f--z+fvfv-y++f--z-fvfv+wfx-fvfv+w++f--z-fvfv+w
  w=fx-fvfv+w++f--z-fvfv+w
  f=
  v=fvfv
}

Dekking6 {
  Angle 4
  Axiom x
  x=fx+fy-fx-fy+
  y=-fx+fy-fx+fy
  f=
}

Dekking2&Med {
  Angle 10
  Axiom k
  k=fl
  l=--fq+fa+++fo-fa-ft
  y=++fp-fa---fn+fa+fu
  n=fv+fa+fy---fa-fs++
  o=fw-fa-fl+++fa+fr--
  p=--g|fp-fa---fn+fa+fu-
  q=++g|fq+fa+++fo-fa-ft+
  r=+fw-fa-fl+++fa+fr|g++
  s=-fv+fa+fy---fa-fs|g--
  t=-fv+fa+fy-
  u=+fw-fa-fl+
  v=+fo-fa-ft+
  w=-fn+fa+fu-
  g=-g++g-
  f=
  a=fa
}
  
Dekking2 { ; Penrose's rhombus tiling
  Angle 10
  Axiom k
  k=fl
  l=--fq++++fo--ft
  y=++fp----fn++fu
  n=fv++fy----fs++
  o=fw--fl++++fr--
  p=--g|fp----fn++fu-
  q=++g|fq++++fo--ft+
  r=+fw--fl++++fr|g++
  s=-fv++fy----fs|g--
  t=-fv++fy-
  u=+fw--fl+
  v=+fo--ft+
  w=-fn++fu-
  g=-g++g-
  f=
}
