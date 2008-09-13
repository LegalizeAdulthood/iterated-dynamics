; The publication "Lindenmayer Systems, Fractals, and Plants",
; by Przemyslaw Prusinkiewicz & James Hanan, Lecture Notes in
; Biomathematics 79, Spinger-Verlag, 1989, gives a number of
; interesting L-system formulations for space-filling curves
; as well as simple plant structures.
;
; L-system definitions created for fractint by Richard Thomson
; <legalize@xmission.com>
;
Fig_2_2 {
  Angle 4
  Axiom F+F+F+F
  F=F+F-F-FF+F+F-F
}

Fig_2_3a {				; Quadratic Koch Island
  Angle 4
  Axiom F+F+F+F
  F=F-FF+FF+F+F-F-FF+F+F-F-FF-FF+F
}

Fig_2_3b {				; Quadratic Snowflake
  Angle 4
  Axiom +F
  F=F-F+F+F-F
}

Fig_2_4 {				; Islands and Lakes
  Angle 4
  Axiom F-F-F-F
  F=F-G+FF-F-FF-F-FG-FF+G-FF+F+FF+FG+FFF
  G=GGGGGG
}

Fig_2_5a {				; Koch Curves 1
  Angle 4
  Axiom F+F+F+F
  F=FF+F+F+F+F+F-F
}

Fig_2_5b {				; Koch Curves 2
  Angle 4
  Axiom F+F+F+F
  F=FF+F+F+F+FF
}

Fig_2_5c {				; Koch Curves 3
  Angle 4
  Axiom F+F+F+F
  F=FF+F-F+F+FF
}

Fig_2_5d {				; Koch Curves 4
  Angle 4
  Axiom F+F+F+F
  F=FF+F++F+F
}

Fig_2_5e {				; Koch Curves 5
  Angle 4
  Axiom F+F+F+F
  F=F+FF++F+F
}

Fig_2_5f {				; Koch Curves 6
  Angle 4
  Axiom F+F+F+F
  F=F+F-F+F+F
}

Fig_2_6 {				; Dragon Curve
  Angle 4
  Axiom FL
  L=L+RF+
  R=-FL-R
}

Fig_2_7a {				; Spacefill Peano
  Angle 4
  Axiom X
  X=XFYFX+F+YFXFY-F-XFYFX
  Y=YFXFY-F-XFYFX+F+YFXFY
}

Fig_2_7b {				; Spacefill Hilbert
  Angle 4
  Axiom X
  X=-YF+XFX+FY-
  Y=+XF-YFY-FX+
}

Fig_2_7c {				; Spacefill Sierpinski
  Angle 4
  Axiom F+XF+F+XF
  X=XF-F+F-XF+F+XF-F+F-X
}

Fig_2_7d {				; Spacefill Quadratic Gosper
  Angle 4
  Axiom -YF
  X=XFX-YF-YF+FX+FX-YF-YFFX+YF+FXFXYF-FX+YF+FXFX+YF-FXYF-YF-FX+FX+YFYF-
  Y=+FXFX-YF-YF+FX+FXYF+FX-YFYF-FX-YF+FXYFYF-FX-YFFX+FX+YF-YF-FX+FX+YFY
}

Fig_2_8a {				; Sierpinski Arrowhead
  Angle 6
  Axiom YF
  X=YF+XF+Y
  Y=XF-YF-X
}

Fig_2_8b {				; Hexagonal Gosper curve
  Angle 6
  Axiom XF
  X=X+YF++YF-FX--FXFX-YF+
  Y=-FX+YFYF++YF+FX--FX-Y
}

Fig_3_2a {				; Plant-like 1
  Angle 14
  Axiom F
  F=F[+F]F[-F]F
}

Fig_3_2b {				; Plant-like 2
  Angle 16
  Axiom X
  X=F-[[X]+X]+F[+FX]-X
  F=FF
}

Fig_3_2c {				; Plant-like 3
  Angle 14
  Axiom Y
  Y=YFX[+Y][-Y]
  X=X[-FFF][+FFF]FX
}

Fig_3_2d {				; Plant-like 4
  Angle 16
  Axiom F
  F=FF+[+F-F-F]-[-F+F+F]
}

Fig_3_2e {				; Plant-like 5
  Angle 18
  Axiom X
  X=F[+X]F[-X]+X
  F=FF
}

Fig_6_1a {				; Hexagonal tiling
  Angle 6
  Axiom X
  X=[-F+F[Y]+F][+F-F[X]-F]
  Y=[-F+F[Y]+F][+F-F-F]
}

Fig_6_1c {				; Spiral tiling
  Angle 24
  Axiom AAAA
  A=X+X+X+X+X+X+
  X=[F+F+F+F[---X-Y]+++++F++++++++F-F-F-F]
  Y=[F+F+F+F[---Y]+++++F++++++++F-F-F-F]
}
