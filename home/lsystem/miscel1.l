CantorDust { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  Angle 6
  Axiom F
  F=FGF
  G=GGG
}

CircularTile { ; Adrian Mariano
  axiom X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X
  x=[F+F+F+F[---X-Y]+++++F++++++++F-F-F-F]
  y=[F+F+F+F[---Y]+++++F++++++++F-F-F-F]
  angle 24
}

Pentagram { ; created by Adrian Mariano
  angle 10
  axiom fx++fx++fx++fx++fx
  ; f=f[++++@1.618033989f]
  x=[++++@i1.618033989f@.618033989f!x!@i.618033989f]
}

SpiralTile { ; by Herb Savage
  ; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers"
  angle 16
  axiom X++X++X++X++|G|X++X++X++X
  X=[C12FX+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
  Y=[C12F+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
}
