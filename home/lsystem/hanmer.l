; SPDX-License-Identifier: GPL-3.0-only
;
adh6894 { ; Anthony Hanmer 2001, a.hanmer@gmail.com
   Angle 8 ; DC from 6829:
   Axiom x ; My only irregular-interior DC so far
   x=f[@iq2-x]|[+!+!@.5x]f[@iq2+x]
}

hc043 {  ;  Hanmer Curve variation
   ;  Anthony Hanmer 2001, a.hanmer@gmail.com
   Angle 4; one of the best
   Axiom |f
   f=f[@.5f]f|f+
}

sq50217r { ; Anthony Hanmer 1/9/2003, a.hanmer@gmail.com
   Angle 4  ; One of my VERY best Hanmer Tiles:
   ; WAY COOL, a personal all-parallel favourite
   Axiom +c53f+g+c12f
   f=GGGGG|FFFFF|GGGGG+G+ffggf|fggff+G+fgfgg|ggfgf+G+ffgfg|gfgff+G+gffgg|ggffg+G+GGGGG+GGGGG+GGGGG
   g=ggggg
}

ST40028* { ; Anthony Hanmer 6/2002, a.hanmer@gmail.com
   Angle 4  ; non-crossing; and 4 together make a tile
   Axiom c9f+c10f+c12f+c14f
   f=f+ff-f-ff+f+ff-f
}

tr20001* { ; Anthony Hanmer 2000, a.hanmer@gmail.com
   Angle 6  ; My 2nd Hanmer Tile
   ; (& 1st 2x2 triangular HT)
   ; I think there are no more 2x2 triangular HTs to be found.
   ; (I am later proved to be SO wrong.)
   ; One of my VERY best HTs
   Axiom c9f++c12f++c14f
   f=f+f|f++f
}
