Bush { ; Adrian Mariano
  Angle 16
  Axiom ++++F
  F=FF-[-F+F+F]+[+F-F-F]
}

Leaf1 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
  ; Compound leaf with alternating branches, Figure 5.12b p.130
  angle 8
  axiom x
  a=n
  n=o
  o=p
  p=x
  b=e
  e=h
  h=j
  j=y
  x=F[+A(4)]Fy
  y=F[-B(4)]Fx
  F=@1.18F@i1.18
}

Leaf2 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
   ; Compound leaf with alternating branches, Figure 5.12a p.130
  angle 8
  axiom a
  a=f[+x]fb
  b=f[-y]fa
  x=a
  y=b
  f=@1.36f@i1.36
}

MyTree { ; Adrian Mariano
  Angle 16
  Axiom ++++F
  F=FF-[XY]+[XY]
  X=+FY
  Y=-FX
}

Plant01 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24a p.25
    ; also p.285a The Science of Fractal Images
  angle 14
  axiom f
  f=F[+F]F[-F]F
}

Plant02 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24b p.25
  angle 18
  axiom f
  f=F[+F]F[-F][F]
}

Plant03 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24c p.25
  angle 16
  axiom f
  f=FF-[-F+F+F]+[+F-F-F]
}

Plant04 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24d p.25
  angle 18
  axiom x
  X=F[+X]F[-X]+X
  F=FF
}

Plant05 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24e p.25
  angle 14
  axiom x
  X=f[+X][-X]FX
  F=FF
}

Plant06 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
    ; Plant-like structure, figure 1.24f p.25
  angle 16
  axiom x
  X=F-[[X]+X]+F[+FX]-X
  F=FF
}

Plant07 { ; Ken Philip, from The Science of Fractal Images p.285b
  axiom Z
  z=zFX[+Z][-Z]
  x=x[-FFF][+FFF]FX
  angle 14
}

Plant08 { ; Ken Philip, from The Science of Fractal Images, p.286
  axiom SLFFF
  s=[+++Z][---Z]TS
  z=+H[-Z]L
  h=-Z[+H]L
  t=TL
  l=[-FFF][+FFF]F
  angle 20
}

Plant09 { ; Adrian Mariano
   axiom y
   x=X[-FFF][+FFF]FX
   y=YFX[+Y][-Y]
   angle 14
}

Plant10 { ; Adrian Mariano
   axiom f
   f=f[+ff][-ff]f[+ff][-ff]f
   angle 10
}

Plant11 { ; Adrian Mariano
   axiom f
   f=F[+F[+F][-F]F][-F[+F][-F]F]F[+F][-F]F
   angle 12
}

Tree1 { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  angle=12;
  axiom +++FX
  X=@.6[-FX]+FX
}
