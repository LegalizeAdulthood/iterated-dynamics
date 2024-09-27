; SPDX-License-Identifier: GPL-3.0-only
;
; Date: Wed, 19 Mar 1997 14:04:40 -0600
; From: "William A McWorter Jr."
;

Norton12 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx
   y=+f-y
   z=|f|z+f-y|f|z-f+w|f|z
   w=-f+wfx+f-yfx-f+w|f|z-f+w
   f=
}

Norton11 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx-f+wfx+f-y
   y=|f|z+f-y
   z=|f|z+f-y|f|z-f+w
   w=fx-f+w|f|z-f+w
   f=
}

Norton10 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx-f+wfx+f-y
   y=|f|z+f-y
   z=|f|z-f+w|f|z+f-y|f|z-f+w
   w=fx-f+w
   f=
}

Norton09 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx-f+wfx
   y=+f-yfx+f-y|f|z-f+w|f|z+f-y
   z=|f|z
   w=-f+w|f|z-f+w
   f=
}

Norton08Med {
   Angle 8
   Axiom +r
   r=x
   x=x-f-y+f+x+f+y
   y=x-f-y-f-x+f+y
}

Norton08 {
   Angle 4
   Axiom r
   r=x            ;  r=c40x<8y<8z<4w
   x=fx-fy+fx+fy- ;  x=fx-f+wfx+f-y
   y=+fx-fy-fx+fy ;  y=|f|z+f-yfx+f-y
   ;  z=|f|z+f-y|f|z-f+w
   ;  w=fx-f+w|f|z-f+w
   f=
}

Norton07Med {
   Angle 8
   Axiom +++r
   r=w
   x=x-f-w+f+x+f+y-f-x
   z=z+f+w-f-z-f-y-f-x+f+y+f+z
   w=w-f-z+f+w
}

Norton07 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx-f+wfx+f-yfx
   y=+f-y
   z=|f|z-f+w|f|z+f-yfx+f-y|f|z
   w=-f+w|f|z-f+w
   f=
}

Norton06 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx-f+w|f|z-f+wfx
   y=+f-yfx+f-y|f|z+f-y
   z=|f|z-f+w|f|z
   w=-f+w
   f=
}

Norton05 {
   Angle 4
   Axiom r
   r=c40x<8y<8z<4w
   x=fx+f-yfx-f+w|f|z-f+wfx+f-yfx
   y=+f-y|f|z+f-y
   z=|f|z
   w=-f+w|f|z-f+w
   f=
}

Norton04Med {
   Angle 8
   Axiom +r
   r=x
   x=x+f+y+f+z-f-y-f-x-f-w+f+x+f+y
   y=z-f-y
   z=z+f+w-f-z+f+w
   w=x-f-w
}

Norton04 {
   Angle 4
   Axiom r
   r=c40xc48yc56zc60w
   x=fx+f-y|f|z+f-yfx-f+wfx+f-y
   y=|f|z+f-y
   z=|f|z-f+w|f|z-f+w
   w=fx-f+w
   f=
}

Norton03Med {
   Angle 8
   Axiom +r
   r=x
   x=x-f-w+f+x+f+y-f-x
   y=y-f-x+f+y
   w=w-f-z+f+w-f-z-f-y+f+z+f+w
}

Norton03 {
   Angle 4
   Axiom r
   r=c40xc48yc56zc60w
   x=fx-f+wfx+f-yfx
   y=+f-yfx+f-y
   z=|f|z
   w=-f+w|f|z-f+w|f|z+f-y|f|z-f+w
   f=
}

Norton02Med {
   Angle 8
   Axiom +r
   r=z
   x=x+f+y
   y=z-f-y-f-x+f+y+f+z-f-y
   z=z+f+w+f+x-f-w-f-z+f+w
   w=x-f-w
}

Norton02 {
   Angle 4
   Axiom r
   r=c40xc48yc56zc60w
   x=fx+f-y
   y=|f|z+f-yfx+f-y|f|z+f-y
   z=|f|z-f+wfx-f+w|f|z-f+w
   w=fx-f+w
   f=
}

Norton01 { ; Vic Norton
   Angle 4
   Axiom r
   r=x
   x=fx-f+wfx
   y=+f-y
   z=|f|z+f-yfx+f-y|f|z
   w=-f+w|f|z-f+w
   f=
}
