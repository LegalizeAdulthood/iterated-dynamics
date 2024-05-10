; I have a couple of cool space-filling hexagonal fractals, plus another
; couple of fractals designed as L-systems that I'd like to see included in
; future Fractint.L files. No, it's not for money, just for glory <grin>.
; Anyway, I was wondering where I could post them or send them so they would
; be considered? In case you are interested, here they are (The last couple
; might not be unique, but hey...):

Hextar { ; "Hexagonal Star" by Michael A. Rouse, 1996
   Angle 6
   Axiom s ; to make things start at
   s=L     ; order 1
   L=LzfR--fR-f++Lf++L-f+Lf+R--y
   R=z++L-fR-f+R--fR--f+Lf++LfyR
   z=-
   y=+
}

Hextarb { ; "Hexagonal Star b" 
   Angle 6 { ; by Michael A. Rouse, 1996
   Axiom s ; to make things start at
   s=Lf++Lf++Lf     ; order 1
   L=LzfR--fR-f++Lf++L-f+Lf+R--y
   R=z++L-fR-f+R--fR--f+Lf++LfyR
   z=-
   y=+
}


HexGasket { ; "Hexagonal Gasket" by Michael A. Rouse
   ; I came up with this one while trying to figure out a cool fractal
   ; antenna. This shape can be drawn without crossing lines or picking
   ; up pen -- in other words, you can make an antenna this shape
   ; without crossing a single wire. It has 6-fold plane symmetry,
   ; six-fold rotational symmetry, symmetric about the origin, and
   ; appears roughly the same at various magnifications. Besides, it's
   ; simple and cool!
   Angle 6
   Axiom F+F+F+F+F+F--
   F=F+F+F--F--F+F+F
}


AntiKochCurve101 { ; Mike Rouse
   Angle 6
   Axiom F
   F=F-F++FF--F+F
}

AntiKochStar { ; Mike Rouse
   Angle 6
   Axiom F--F--F
   F=F-F++FF--F+F
}
