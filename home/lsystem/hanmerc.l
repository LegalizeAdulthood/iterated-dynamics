; ... In the course of learning more about Hausdorff dimension from Morgan L. 
; Owens, I have also been instructed in simplifying my L-systems containing 
; the unnecessary "undefined x".
;
; So, here is the replacement for hanmerc.l, which I recently posted, 
; containing Morgan's better versions of my 2 favourite discoveries.  Please 
; name this file hanmerc.l and remove the previos one.
;
; Tony Hanmer,
;
; Who still has a lot to learn about L-systems
;

ADH107h2o { ; Anthony Hanmer 2000
   Angle 8 ; Original version needing improvement
   Axiom f
   f=+f+x--x+f+x++x+f+
}

ADH107h2 {  ; Anthony Hanmer 2000, MUCH simplified by Morgan L. Owens
   Angle 4   ; Who says L-systems are boring?!
   Axiom ++f ; I like this one so much, I'll name it after myself:
   f=ff++f+  ; the Hanmer Curve, variant 1
}         ; Fractal dimension, calculated by Morgan:  1.3652

ADH107h2a1 { ; Anthony Hanmer 2000
   Angle 4 ; Even orders have 90-degree rotational symmetry
   Axiom f+f+f+f ; Odd orders have glide symmetry
   f=ff++f+
}

ADH107h2a1col { ; Anthony Hanmer 2000
   Angle 4 ; Even orders have 90-degree rotational symmetry
   Axiom c09f+c12f+c10f+c13f ; Odd orders have glide symmetry
   f=ff++f+
}

ADH107h2a2 { ; Anthony Hanmer 2000
   Angle 4 ;
   Axiom fff+f+fff+f
   f=ff++f+
}

ADH107h2c { ; Anthony Hanmer 2000
   Angle 4 ; Even orders are x-shaped, odd orders are square
   Axiom ff--ff+ff--ff+ff--ff+ff--ff
   f=ff++f+
}

ADH152o { ; Anthony Hanmer 2000
   Angle 4 ; Original version needing improvement
   Axiom ++f
   f=f--x+f++x-f+x
}

ADH152 {    ; Anthony Hanmer 2000, MUCH simplified by Morgan L. Owens
   Angle 4   ; A relative of ADH107h2
   Axiom ++f ; so we'll call it the Hanmer Curve, variant 2
   f=f-f+f+  ; Fractal dimension, calculated by Morgan:  1.3652
}

ADH152a { ; Anthony Hanmer 2000
   Angle 4 ; Even orders have 180-degree rotational symmetry
   Axiom f++f ; Odd orders are asymmetrical
   f=f-f+f+
}

ADH152b { ; Anthony Hanmer 2000
   Angle 4 ; Even orders have 180-degree rotational symmetry
   Axiom ff--ff ; Odd orders are asymmetrical
   f=f-f+f+
}

ADH152c { ; Anthony Hanmer 2000
   Angle 4 ; Even orders are long, odd orders are square
   Axiom fff+f+fff+f
   f=f-f+f+
}

ADH152d { ; Anthony Hanmer 2000
   Angle 4 ; Even orders are x-shaped
   Axiom ff--ff+ff--ff+ff--ff+ff--ff ; Odd orders are square
   f=f-f+f+
}
