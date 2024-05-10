;
; Spiral L-systems José Torres Torres 1998
;

espiral2 {
   Angle 46
   Axiom X
   X=X>1G+@.997X[+++FF]
}

espiral3 {
   Angle 49
   Axiom X
   X=XG+@.98X[-----------FFFFFFF]
}

espiral4 {
   Angle 49
   Axiom X
   X=XG+@.99X[-----------F+F+F+F+F+F]
}

1Nautilus { ;Nautilus tipo primitivo
   Angle 49
   Axiom X
   X=XG+@.987X[-------------F+F+F++F+++F++++F+++++F++++++F]
}

2Nautilus {
   ;Nautilus white Palette rotates only in the edges
   Angle 49
   Axiom C255X
   X=XG+@.987X[\120<1F-<1F-<1F-<1F-<1F-<1F/120+<1F+<1F+<1F+++++++<1F+<1F+<1F]
}

3Nautilus {;Nautilus Rotation of paletes on both axis
   Angle 49
   Axiom C255X
   X=X<1G+@.9879X[\120<1F-<1F-<1F-<1F-<1F-<1F/120+<1F+<1F+<1F+++++++<1F+<1F+<1F]
}

4Nautilus {;Nautilus  rotaci¢n color bi-direccional
   Angle 47
   Axiom X
   X=XG+@.97X[@3.68A@.92B]
   A=--------------<1F-<1F-<1F-<1F-<1F-<1@.5F/120+<1F+<1F+<1F
   B=\2++++++++++++++<1FF+<1FF+<1FF+<1FF+<1FF
}

5Nautilus {;Nautilus plain white
   Angle 47
   Axiom X
   X=XG+@.97X[@3.68A@.92B]
   A=--------------F-F-F-F-F-@.5F/120+F+F+F
   B=\2++++++++++++++FF+FF+FF+FF+FF
}

1heptagon {
   angle 7
   Axiom +[X]+[X]+[X]+[X]+[X]+[X]+[X]
   X=F-[>1@.4X]+[>1@.4X]+[>1@.4X]
}

1Hexagon {
   ANGLE 6
   Axiom +[X]+[X]+[X]+[X]+[X]+[X]
   X=F-[>1@.4X]-[>1@.25X]++[>1@.4X]+[>1@.4X]+[>1@.25X]
}

1SPIRAL {    ;LOGARITHMIC SPIRAL 3 BRANCHES
   ANGLE 36
   AXIOM ++++++++++++[X]++++++++++++[X]++++++++++++[X]
   X=>1F[+@.95X][+++++@.4Y]+@.95G
   Y=>1F[+@.95Y]
}

2SPIRAL {    ;LOGARITHMIC SPIRAL 3 BRANCHES 
   ANGLE 36
   AXIOM ++++++++++++[X]++++++++++++[X]++++++++++++[X]
   X=>1F[+@.95X][+++++@.6Y]+@.95G
   Y=Y>1+@.95F
}

3SPIRAL {    ;ANOTHER TYPE
   ANGLE 42
   AXIOM ++++++++++++++[X]++++++++++++++[X]++++++++++++++[X]
   X=>1F[+@.95X][++++++++@.4Y]+@.95G
   Y=>1F[+@.95Y]
}

4SPIRALS { 
   ANGLE 42
   AXIOM ++++++++++++++[X]++++++++++++++[X]++++++++++++++[X]
   X=>1F[+@.95X][++++++++@.4Y]+@.95G
   Y=>1F[+@.95Y][++++++++@.4Z]+@.95G
   Z=>1F[+@.95X]
}

LEAF { 
   ;THE VERY COMPLEX SELF-RECURSIVE
   ;LEAF FRACTAL BY JOSE TORRES 1998
   ;'DO NOT RECOMMEND GOING FURTHER THAN ORDER 30
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][++++++++@.4Y]+@.95G
   Y=>1F[+@.95Y][++++++++@.4X]+@.95G
}

LEAF_REVERSE { 
   ;THE VERY COMPLEX SELF-RECURSIVE
   ;LEAF FRACTAL BY JOSE TORRES 1998
   ;'DO NOT RECOMMEND GOING FURTHER THAN ORDER 30
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][--------@.4X]+@.95G
}

LEAF_DOUBLE { 
   ;THE VERY COMPLEX SELF-RECURSIVE
   ;LEAF FRACTAL BY JOSE TORRES 1998
   ;'DO NOT RECOMMEND GOING FURTHER THAN ORDER 20
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G+@.95[[++++++++@.4Y]]+@.9025G
   Y=>1F[+@.95Y][++++++++@.4X]+@.95G+@.95[[--------@.4X]]+@.9025G
}

LEAF_L6 { ;Reduced complexity set to 6 levels
   ;Order 40-50 can be easily achieved
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][--------@.4Z]+@.95G
   Z=>1F[+@.95Z][--------@.4U]+@.95G
   U=>1F[+@.95U][--------@.4V]+@.95G
   V=>1F[+@.95V][--------@.4W]+@.95G
   W=>1F[+@.95W]
}

LEAF_L6ALTERN { ;Reduced complexity set to 5 levels
   ;Order 40-50 can be easily achieved
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][++++++++@.4Z]+@.95G
   Z=>1F[+@.95Z][--------@.4U]+@.95G
   U=>1F[+@.95U][++++++++@.4V]+@.95G
   V=>1F[+@.95V][--------@.4W]+@.95G
   W=>1F[+@.95W]
}

LEAF_L5 { ;Reduced complexity set to 5 levels
   ;Order 60-70 can be easily achieved
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][--------@.4Z]+@.95G
   Z=>1F[+@.95Z][--------@.4U]+@.95G
   U=>1F[+@.95U][--------@.4W]+@.95G
   W=>1F[+@.95W]
}

LEAF_L4 { ;Reduced complexity set to 4 levels
   ;Order >70 can be easily achieved
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][--------@.4Z]+@.95G
   Z=>1F[+@.95Z][--------@.4U]+@.95G
   U=>1F+@.95U
}

LEAF_L3 { ;Reduced complexity set to 3 levels
   ;I reached order 94 in a short time
   ;for higher orders I had out of memory error
   ANGLE 42
   AXIOM X
   X=>1F[+@.95X][--------@.4Y]+@.95G
   Y=>1F[+@.95Y][--------@.4U]+@.95G
   U=>1F+@.95U
}
