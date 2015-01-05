adh6894 { ; Anthony Hanmer 2001, a.hanmer@gmail.com
  Angle 8 ; DC from 6829:
  Axiom x ; My only irregular-interior DC so far
  ; More work to be found at:
  ; http://spanky.triumf.ca/pub/fractals/LSYSTEMS/
  ; in TDC.l
  x=f[@iq2-x]|[+!+!@.5x]f[@iq2+x]
  }

hc043 {  ;  Hanmer Curve variation
         ;  Anthony Hanmer 2001, a.hanmer@gmail.com
  angle 4; one of the best
  ; More work to be found at:
  ; http://spanky.triumf.ca/pub/fractals/LSYSTEMS/
  ; in HCVar.l
  axiom |f
  f=f[@.5f]f|f+
  }

sq50217r { ; Anthony Hanmer 1/9/2003, a.hanmer@gmail.com
  Angle 4  ; One of my VERY best Hanmer Tiles:
  ; WAY COOL, a personal all-parallel favourite
  ; More work to be found at:
  ; http://spanky.triumf.ca/pub/fractals/LSYSTEMS/
  ; in HanmerTiles.zip
  Axiom +c53f+g+c12f
  f=GGGGG|FFFFF|GGGGG+G+ffggf|fggff+G+fgfgg|ggfgf+G+ffgfg|gfgff+G+gffgg|ggffg+G+GGGGG+GGGGG+GGGGG
  g=ggggg
  }

ST40028* { ; Anthony Hanmer 6/2002, a.hanmer@gmail.com
  Angle 4  ; non-crossing; and 4 together make a tile
  ; More work to be found at:
  ; http://spanky.triumf.ca/pub/fractals/LSYSTEMS/
  ; in STiles.zip
  Axiom c9f+c10f+c12f+c14f
  f=f+ff-f-ff+f+ff-f
  }

tr20001* { ; Anthony Hanmer 2000, a.hanmer@gmail.com
  Angle 6  ; My 2nd Hanmer Tile
  ; (& 1st 2x2 triangular HT)
  ; I think there are no more 2x2 triangular HTs to be found.
  ; (I am later proved to be SO wrong.)
  ; One of my VERY best HTs
  ; More work to be found at:
  ; http://spanky.triumf.ca/pub/fractals/LSYSTEMS/
  ; in HanmerTiles.zip
  Axiom c9f++c12f++c14f
  f=f+f|f++f
  }
