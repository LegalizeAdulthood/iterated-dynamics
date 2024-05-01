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
  x=[++++@i1.618033989f@.618033989f!x!@i.618033989f]
}

Sphinx { ; by Herb Savage
  ; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers"
  ; This is an example of a "reptile"
  Angle 6
  Axiom X
  X=+FF-YFF+FF--FFF|X|F--YFFFYFFF|
  Y=-FF+XFF-FF++FFF|Y|F++XFFFXFFF|
  F=GG
  G=GG
}

SpiralTile { ; by Herb Savage
  ; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers"
  angle 16
  axiom X++X++X++X++|G|X++X++X++X
  X=[C12FX+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
  Y=[C12F+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
}

Vertigo1 { ; by Morgan Savage
  ; Try order 13 and color cycle in both 256 and 16 color modes
  Angle 46
  Axiom X
  X=X>1F+@.9997X
}

Vertigo2 { ; by Morgan Savage
  ; Try order 13 and color cycle in both 256 and 16 color modes
  Angle 49
  Axiom X
  X=X>1F+@.9997X
}

VoderbergTile { ; by Herb Savage
  ; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
  ; A spiral tiling by Heinz Voderberg
  angle 30
  axiom \84.1A\96@4.783386117M@I4.783386117/96A
  A=X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12Z
  X=[D\78D\46.37236@3.393427D@I3.393427/46.37236D\114[\168X\24Y]D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D]
  Y=[D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D\168[\192Y]D\78D\46.37236@3.393427D@I3.393427/46.37236D]
  Z=[D\78D\46.37236@3.393427D@I3.393427/46.37236D\114D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D]
}
