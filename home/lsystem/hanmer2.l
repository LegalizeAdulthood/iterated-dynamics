;  From: "Anthony (Tony) Hanmer" <ahanmer@azerin.com>
;  To: <fractint@lists.xmission.com>
;  Subject: (fractint) More L-Systems
;  Date: Thu, 18 Nov 1999 11:05:28 +0400
;  X-MimeOLE: Produced By Microsoft MimeOLE V4.71.1712.3
;
;  Rather quiet par-wise lately!
;
;  Here are some more L-systems; I'm starting to get more control, though still
;  a long way from fluency in this syntax, though also still enjoying its
;  restrictions and possibilities very much.
;
;  ADH105d is a right-angled version of Terdragon, with seemingly identical
;  results when filled in solid.  Try as I did I couldn't produce the median
;  and rounded versions of this, but William McWorter, writer of the L-systems
;  tutorial, kindly helped me with these and several other problems.  Included
;  are some variations on his Border; a 4-colour set of rounded Dragon Curves
;  which proves that they can nestle with endpoints touching and not overlap;
;  and a similar thing for the rounded square Terdragon, plus assorted odds and
;  ends.  Hope someone finds these interesting.
;
;  Anthony Hanmer

Border1a { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4
  Axiom XYXYXYX+XYXYX+XYXYXYX+XYXYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1b { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4
  Axiom XYXYXYXYX+XYXYX+XYXYXYXYX+XYXYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1c { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4
  Axiom XYXYXYXYXYX+XYXYX+XYXYXYXYXYX+XYXYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1d { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ; Square spiral
  Axiom XYXYXYXYXYX+XYXYXYXYXYX+XYXYXYXYXYX+XYXYXYXYX+XYXYXYXYX+XYXYXYX+XYXYXYX+XYXYX+XYXYX+XYX+XYX+X+X
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1f { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ; Back & forth, inner gaps filled
  Axiom XYX+X+XYX-X-XYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1g { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+X+XYX+X
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1h { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+XYX+XYX+XYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1i { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+X+XYX+X
  F=
  X=FX+GX+FXFY-GY-
  Y=+FX+GXFY-FY-GY
}

Border1j { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+XYX+XYX+XYX
  F=
  X=FX+GX+FXFY-GY-
  Y=+FX+GXFY-FY-GY
}

Border1k { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+X+XYX+X
  F=
  X=GX+FX+FXGY-FY-
  Y=+GX+FXFY-GY-FY
}

Border1l { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX+XYX+XYX+XYX
  F=
  X=GX+FX+FXGY-FY-
  Y=+GX+FXFY-GY-FY
}

Border1m { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX-X-XYX-X
  F=
  X=FX+GX+FXFY-GY-
  Y=+FX+GXFY-FY-GY
}

Border1n { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX-XYX-XYX-XYX
  F=
  X=FX+GX+FXFY-GY-
  Y=+FX+GXFY-FY-GY
}

Border1o { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX-X-XYX-X
  F=
  X=GX+FX+FXGY-FY-
  Y=+GX+FXFY-GY-FY
}

Border1p { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XYX-XYX-XYX-XYX
  F=
  X=GX+FX+FXGY-FY-
  Y=+GX+FXFY-GY-FY
}

Border1q { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XY+XY+XY+XY
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1r { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom XY+YX+XY+YX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Border1s { ; William McWorter, altered by Anthony Hanmer 1999
  Angle 4  ;
  Axiom X+Y+X+Y
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

ADH70d { ; Anthony Hanmer 1999
  Angle 6
  Axiom x+x+x+x+x+x-x+x+x+x+x+x-x+x+x+x+x+x-x+x+x+x+x+x-x+x+x+x+x+x-x+x+x+x+x+x
  x=fx-x++x+x-x-x
}

ADH70e { ; Anthony Hanmer 1999
  Angle 6
  Axiom x-x-x-x-x-x+x-x-x-x-x-x+x-x-x-x-x-x
  x=fx-x++x+x-x-x
}

ADH75f { ; Anthony Hanmer 1999
  Angle 4
  Axiom x+x+x+x+x-x+x+x+x+x-x+x+x+x+x-x+x+x+x+x
  x=fx-x+x+x-x
}

ADH75g { ; Anthony Hanmer 1999
  Angle 4
  Axiom x-x-x-x-x+x-x-x-x-x+x-x-x-x-x+x-x-x-x-x
  x=fx-x+x+x-x
}

ADH99m { ; Anthony Hanmer 1999
  Angle 4
  Axiom xyx+xyx+xyx+xyx
  x=fyyxx+
  y=fxxyy-
}

ADH100 { ; Anthony Hanmer 1999
  Angle 8
  Axiom x+x+x+x+x+x+x+x
  x=fxx+xx+xx+xx+
}

ADH100b { ; Anthony Hanmer 1999
  Angle 8
  Axiom x+x+x+x+x+x+x+x
  x=fx+x-x+x
}

ADH100c { ; Anthony Hanmer 1999
  Angle 8
  Axiom x-x-x-x-x-x-x-x
  x=fx+x-x+x
}

ADH100d { ; Anthony Hanmer 1999
  Angle 8
  Axiom x+x+x+x+x+x+x+x
  x=fx+x-x-x+x
}

ADH100e { ; Anthony Hanmer 1999
  Angle 8
  Axiom x-x-x-x-x-x-x-x
  x=fx+x-x-x+x
}

DragR2c { ; Rounded Dragon variation 1
  Angle 8 ; 2 dragons joined end to end, 2 colours
  Axiom c10x++c12u++ ; Anthony Hanmer 1999
  f=
  x=fx+@.5fz@2+fy
  y=fx-@.5fz@2-fy
  z=fz
  u=fu+@.5fw@2+fv
  v=fu-@.5fw@2-fv
  w=fw
}

DragR4c { ; Rounded Dragon variation 4, Anthony Hanmer and William McWorter
  Angle 8 ; 4 dragons in a cross, 4 colours
  Axiom [c09-@.5z@2+x]---@.5g@2++++[c10@.5z@2+u]--@.5g@2++++[c11@.5z@2+r]--@.5g@2++++[c12@.5z@2+o]
  f=
  x=fx+@.5fz@2+fy
  y=fx-@.5fz@2-fy
  z=fz
  u=fu+@.5fw@2+fv
  v=fu-@.5fw@2-fv
  w=fw
  r=fr+@.5ft@2+fs
  s=fr-@.5ft@2-fs
  t=ft
  o=fo+@.5fq@2+fp
  p=fo-@.5fq@2-fp
  q=fq
}

ADH102 { ; Anthony Hanmer 1999
  Angle 4
  Axiom F
  F=F-F+F+FF-F-F+F
}

ADH102a { ; Anthony Hanmer 1999
  Angle 4
  Axiom F-F-F-F
  F=F-F+F+FF-F-F+F
}

ADH103 { ; Anthony Hanmer 1999
  Angle 4
  Axiom F
  F=FF+F+FF-F-FF
}

ADH103a { ; Anthony Hanmer 1999
  Angle 4
  Axiom F-F-F-F
  F=FF+F+FF-F-FF
}

ADH105d { ; Anthony Hanmer 1999
  Angle 4 ; Right-angled version of Terdragon
  Axiom f ; Produces identical results when filled in solid
  f=f+f+f-f-f
}

ADH105h { ; Anthony Hanmer 1999
  Angle 4 ;
  Axiom f+f+f+f
  f=f+f-f
}

ADH105m { ; median version of ADH105d
  Angle 8 ; altered by William McWorter
  Axiom -x
  x=x+f+x+f+x-f-x-f-x
}

ADH105r { ; rounded version of ADH105d
  Angle 8 ; altered by William McWorter
  Axiom x
  f=
  y=fy
  x=fx+@.5fy@2+fx+@.5fy@2+fx-@.5fy@2-fx-@.5fy@2-fx
}

ADH105r2 { ; rounded version of ADH105d, 4 joined mono
  Angle 8 ; rounding by William McWorter
  Axiom x++x++x++x
  f=
  y=fy
  x=fx+@.5fy@2+fx+@.5fy@2+fx-@.5fy@2-fx-@.5fy@2-fx
}

ADH105r3 { ; rounded version of ADH105d, 4 joined coloured
  Angle 8 ; rounding by William McWorter
  Axiom c09x++c10v++c11t++c12r
  f=
  y=fy
  x=fx+@.5fy@2+fx+@.5fy@2+fx-@.5fy@2-fx-@.5fy@2-fx
  w=fw
  v=fv+@.5fw@2+fv+@.5fw@2+fv-@.5fw@2-fv-@.5fw@2-fv
  u=fu
  t=ft+@.5fu@2+ft+@.5fu@2+ft-@.5fu@2-ft-@.5fu@2-ft
  s=fs
  r=fr+@.5fs@2+fr+@.5fs@2+fr-@.5fs@2-fr-@.5fs@2-fr
}
