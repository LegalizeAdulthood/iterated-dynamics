
ColorTriangGasket { ; Adrian Mariano
  Angle 6
  Axiom --X
  X=++FXF++FXF++FXF>1
  F=FF
}

Island1 { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  angle 4
  axiom F+F+F+F
  F=FFFF-F+F+F-F[-GFF+F+FF+F]FF
  G=@8G@I8
}

Island2 { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  angle 4
  axiom f+f+f+f
  f=f+gf-ff-f-ff+g+ff-gf+ff+f+ff-g-fff
  g=@6G@I6
}

Sierpinski2 { ; from The Science of Fractal Images
  axiom FXF--FF--FF
  f=FF
  x=--FXF++FXF++FXF--
  angle 6
}

Sierpinski3 { ; From Jim Hanan via Corbit
  axiom F-F-F
  f=F[-F]F
  angle 3
}

SierpinskiSquare { ; Adrian Mariano
  Angle 4
  Axiom F+F+F+F
  F=FF+F+F+F+FF
}
