; The publication "Lindenmayer Systems, Fractals, and Plants",
; by Przemyslaw Prusinkiewicz & James Hanan, Lecture Notes in
; Biomathematics 79, Spinger-Verlag, 1989, gives a number of
; interesting L-system formulations for space-filling curves
; as well as simple plant structures.
;
; L-system definitions created for Iterated Dynamics
; by Richard Thomson
;
Quad_Koch_Isle1 {   ; quadratic Koch island 1
   Angle 4           ; Figure 2.2(a), order 0
   Axiom F+F+F+F     ; Figure 2.2(b), order 1
   F=F+F-F-FF+F+F-F  ; Figure 2.2(c), order 2
}                   ; Figure 2.2(d), order 3

Quad_Koch_Isle2 {   ; quadratic Koch island 2
   Angle 4           ; Figure 2.3(a), order 2
   Axiom F+F+F+F
   F=F-FF+FF+F+F-F-FF+F+F-F-FF-FF+F
}

Quad_Snowflake {    ; quadratic snowflake
   Angle 4           ; Figure 2.3(b), order 4
   Axiom +F
   F=F-F+F+F-F
}

Islands_n_Lakes {   ; islands and lakes
   Angle 4           ; Figure 2.4, order 2
   Axiom F-F-F-F
   F=F-G+FF-F-FF-FG-FF+G-FF+F+FF+FG+FFF
   G=GGGGGG
}

Koch_Curves_1 {     ; Figure 2.5(a), order 4
   Angle 4
   Axiom F+F+F+F
   F=FF+F+F+F+F+F-F
}

Koch_Curves_2 {     ; Figure 2.5(b), order 4
   Angle 4
   Axiom F+F+F+F
   F=FF+F+F+F+FF
}

Koch_Curves_3 {     ; Figure 2.5(c), order 3
   Angle 4
   Axiom F+F+F+F
   F=FF+F-F+F+FF
}

Koch_Curves_4 {     ; Figure 2.5(d), order 4
   Angle 4
   Axiom F+F+F+F
   F=FF+F++F+F
}

Koch_Curves_5 {     ; Figure 2.5(e), order 5
   Angle 4
   Axiom F+F+F+F
   F=F+FF++F+F
}

Koch_Curves_6 {     ; Figure 2.5(f), order 4
   Angle 4
   Axiom F+F+F+F
   F=F+F-F+F+F
}

Dragon_Curve {      ; Figure 2.6, order 14
   Angle 4
   Axiom FL
   L=L+RF+
   R=-FL-R
}

Space_Peano {       ; spacefilling Peano curve
   Angle 4           ; Figure 2.7(a), order 3
   Axiom X
   X=XFYFX+F+YFXFY-F-XFYFX
   Y=YFXFY-F-XFYFX+F+YFXFY
}

Space_Hilbert {     ; spacefilling Hilbert curve
   Angle 4           ; Figure 2.7(b), order 5
   Axiom X
   X=-YF+XFX+FY-
   Y=+XF-YFY-FX+
}

Space_Sierpnski {   ; spacefilling Sierpinski curve
   Angle 4           ; Figure 2.7(c), order 4
   Axiom F+XF+F+XF
   X=XF-F+F-XF+F+XF-F+F-X
}

Space_Qad_Gospr {   ; spacefilling quadratic Gosper curve
   Angle 4           ; Figure 2.7(d), order 2
   Axiom -YF
   X=XFX-YF-YF+FX+FX-YF-YFFX+YF+FXFXYF-FX+YF+FXFX+YF-FXYF-YF-FX+FX+YFYF-
   Y=+FXFX-YF-YF+FX+FXYF+FX-YFYF-FX-YF+FXYFYF-FX-YFFX+FX+YF-YF-FX+FX+YFY
}

Sierpinski_Arwhd {  ; Sierpinski arrowhead
   Angle 6           ; Figure 2.8(a), order 6
   Axiom YF
   X=YF+XF+Y
   Y=XF-YF-X
}

Hex_Gosper_curv {   ; hexagonal Gosper curve
   Angle 6           ; Figure 2.8(b), order 4
   Axiom XF
   X=X+YF++YF-FX--FXFX-YF+
   Y=-FX+YFYF++YF+FX--FX-Y
}

Plant-like_1 {      ; Figure 3.2(a), order 5
   Angle 14
   Axiom F
   F=F[+F]F[-F]F
}

Plant-like_2 {      ; Figure 3.2(b), order 5
   Angle 16
   Axiom X
   X=F-[[X]+X]+F[+FX]-X
   F=FF
}

Plant-like_3 {      ; Figure 3.2(c), order 6
   Angle 14
   Axiom Y
   Y=YFX[+Y][-Y]
   X=X[-FFF][+FFF]FX
}

Plant-like_4 {      ; Figure 3.2(d), order 4
   Angle 16
   Axiom F
   F=FF+[+F-F-F]-[-F+F+F]
}

Plant-like_5 {      ; Figure 3.2(e), order 7
   Angle 18
   Axiom X
   X=F[+X]F[-X]+X
   F=FF
}

Hexagonal_tile {    ; Figure 6.1(a), order 10
   Angle 6
   Axiom X
   X=[-F+F[Y]+F][+F-F[X]-F]
   Y=[-F+F[Y]+F][+F-F-F]
}

Spiral_tiling {     ; Figure 6.1(c), order 5
   Angle 24
   Axiom AAAA
   A=X+X+X+X+X+X+
   X=[F+F+F+F[---X-Y]+++++F++++++++F-F-F-F]
   Y=[F+F+F+F[---Y]+++++F++++++++F-F-F-F]
}
