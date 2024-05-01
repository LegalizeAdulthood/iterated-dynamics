; Tony Hanmer

README {  ; Anthony Hanmer 2000
  Angle 4 ; These are all L-systems which tile part of a polygon -
  Axiom   ; a square or equilateral triangle.  So, putting 3 or 4
  f=ff    ; of them together fills the polygon completely.
}         ; (They're also a subset of Horizons.l.)

ADH236a { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 1st Dragon (2 colours)
  Axiom c9f+c12f+c9f+c12f
  f=ff+f+f++ff++f+f+f
}

ADH236b { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 1st Dragon (4 colours)
  Axiom c9f+c10f+c12f+c14f
  f=ff+f+f++ff++f+f+f
}

ADH236c { ; Anthony Hanmer 2000
  Angle 4 ; rectangle, 2 colours
  Axiom +c9f+c12f+c9f+c12ff+c9f+c12f+c9f-c12f-c9f-c12ff-c9f-c12f
  f=ff+f+f++ff++f+f+f
}

ADH236d { ; Anthony Hanmer 2000
  Angle 4 ; equal-armed cross, 2 colours
  Axiom c9f+c12f++c12f+c9f++c9f+c12f++c12f+c9f
  f=ff+f+f++ff++f+f+f
}

ADH236e { ; Anthony Hanmer 2000
  Angle 4 ; diamond, mono
  Axiom f+f+f+f++f-f-f-ff-f-f+f-f-f+f-f-f+f-f-f++f+f+f-f+f+f-f+f+f-f+f+f
  f=ff+f+f++ff++f+f+f
}

ADH236f { ; Anthony Hanmer 2000
  Angle 4 ; diamond, 2 colours
  Axiom c9f+c12f+c9f+c12f++c12f-c9f-c12f-c9fc12f-c9f-c12f+c9f-c12f-c9f+c12f-c9f-c12f+c9f-c12f-c9f++c9f+c12f+c9f-c12f+c9f+c12f-c9f+c12f+c9f-c12f+c9f+c12f
  f=ff+f+f++ff++f+f+f
}

ADH271a { ; Anthony Hanmer 2000
  Angle 6 ; Escher's 2nd Dragon (3 colours),
  Axiom c9f++c12f++c14f ; triangular tiling
  f=f+f+++f++f
}

ADH271b { ; Anthony Hanmer 2000
  Angle 6 ;
  Axiom c9f++c12f++c14f+++c14f--c12f--c9f
  f=f+f+++f++f
}

ADH271c { ; Anthony Hanmer 2000
  Angle 6 ; Hexagon 1
  Axiom c9f++c14f++c12f---c9f++c14f++c12f---c9f++c14f++c12f---c9f++c14f++c12f---c9f++c14f++c12f---c9f++c14f++c12f
  f=f+f+++f++f
}

ADH271d { ; Anthony Hanmer 2000
  Angle 6 ; Hexagon 2
  Axiom c9f++c14f++c12f---c12f++c14f++c9f---c9f++c14f++c12f---c12f++c14f++c9f---c9f++c14f++c12f---c12f++c14f++c9f
  f=f+f+++f++f
}

ADH271e { ; Anthony Hanmer 2000
  Angle 6 ; Hexagon 3
  Axiom c14f+f+f+f+f+f+++f-f-f-f-f-f--c9f--c12f+++c12f--c9f+++c9f--c12f+++c12f--c9f+++c9f--c12f+++c12f--c9f
  f=f+f+++f++f
}

ADH489b { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 3rd Dragon, 2 colours
  Axiom c9f+c12f+c9f+c12f
  f=f+f+f++f+f++ff+ff
}

ADH489c { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 3rd Dragon, 4 colours
  Axiom c9f+c10f+c12f+c14f
  f=f+f+f++f+f++ff+ff
}

ADH489d { ; Anthony Hanmer 2000
  Angle 4 ; ... further expansion, 2 colours
  Axiom c9f+c12f+c9f+c12f++c12f-c9f-c12f-c9f
  f=f+f+f++f+f++ff+ff
}

ADH489e { ; Anthony Hanmer 2000
  Angle 4 ; rectangle, 2 colours
  Axiom +c9f+c12f+c9f+c12ff+c9f+c12f+c9f-c12f-c9f-c12ff-c9f-c12f
  f=f+f+f++f+f++ff+ff
}

ADH489f { ; Anthony Hanmer 2000
  Angle 4 ; equal-armed cross, 2 colours
  Axiom c9f+c12f++c12f+c9f++c9f+c12f++c12f+c9f
  f=f+f+f++f+f++ff+ff
}

ADH489g { ; Anthony Hanmer 2000
  Angle 4 ; diamond, mono
  Axiom f+f+f+f++f-f-f-ff-f-f+f-f-f+f-f-f+f-f-f++f+f+f-f+f+f-f+f+f-f+f+f
  f=f+f+f++f+f++ff+ff
}

ADH489h { ; Anthony Hanmer 2000
  Angle 4 ; diamond, 2 colours
  Axiom c9f+c12f+c9f+c12f++c12f-c9f-c12f-c9fc12f-c9f-c12f+c9f-c12f-c9f+c12f-c9f-c12f+c9f-c12f-c9f++c9f+c12f+c9f-c12f+c9f+c12f-c9f+c12f+c9f-c12f+c9f+c12f
  f=f+f+f++f+f++ff+ff
}

ADH494a { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 4th Dragon, 2 colours
  Axiom c9f+c12f+c9f+c12f
  f=f+ff++ff+f+f++f+f
}

ADH494b { ; Anthony Hanmer 2000
  Angle 4 ; Escher's 4th Dragon, 4 colours
  Axiom c9f+c10f+c12f+c14f
  f=f+ff++ff+f+f++f+f
}

ADH494c { ; Anthony Hanmer 2000
  Angle 4 ; rectangle, 2 colours
  Axiom +c9f+c12f+c9f+c12ff+c9f+c12f+c9f-c12f-c9f-c12ff-c9f-c12f
  f=f+ff++ff+f+f++f+f
}

ADH494d { ; Anthony Hanmer 2000
  Angle 4 ; equal-armed cross, 2 colours
  Axiom c9f+c12f++c12f+c9f++c9f+c12f++c12f+c9f
  f=f+ff++ff+f+f++f+f
}

ADH494e { ; Anthony Hanmer 2000
  Angle 4 ; diamond, mono
  Axiom f+f+f+f++f-f-f-ff-f-f+f-f-f+f-f-f+f-f-f++f+f+f-f+f+f-f+f+f-f+f+f
  f=f+ff++ff+f+f++f+f
}

ADH494f { ; Anthony Hanmer 2000
  Angle 4 ; diamond, 2 colours
  Axiom c9f+c12f+c9f+c12f++c12f-c9f-c12f-c9fc12f-c9f-c12f+c9f-c12f-c9f+c12f-c9f-c12f+c9f-c12f-c9f++c9f+c12f+c9f-c12f+c9f+c12f-c9f+c12f+c9f-c12f+c9f+c12f
  f=f+ff++ff+f+f++f+f
}
