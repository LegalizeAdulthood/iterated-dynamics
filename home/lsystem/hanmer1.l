;  From: "Anthony (Tony) Hanmer" <ahanmer@azerin.com>
;  To: <fractint@lists.xmission.com>
;  Subject: (fractint) L-system tinkerings
;  Date: Tue, 16 Nov 1999 11:09:42 +0400
;
;  I've spent the last few days discovering L-systems, which seem to be
;  incredibly forgiving of amateurish attempts to get a good result.  After
;  reading the tutorial, I just dove in with a text editor and made lots of
;  mistakes but also found many interesting new critters.  Here are some of
;  them.  My favourites are:
;
;  28f, m, o, s; 33a, 34a, 68, 70, 73c, 75, 76a, d, f; 78, 83a, 84a, 86, 93,
;  99d & h.  Enjoy.
;

ADH04b { ; Anthony Hanmer 1999
   Angle 8
   Axiom +X

   X=f[@.9+x]-f[@.1+x]-f[@.9+x]g[@.9+x]-f[@.1+x]-f[@.9+x]g[@.9+x]-f[@.1+x]-f[@.9+x]g[@.9+x]-f[@.1+x]-f[@.9+x]
}

ADH05a { ; Anthony Hanmer 1999
   Angle 8
   Axiom X

   X=f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]
}

ADH05d { ; Anthony Hanmer 1999
   Angle 8
   Axiom X

   X=f[@.7+x]-f[@.2+x]-f[@.7+x]g[@.7+x]-f[@.2+x]-f[@.7+x]g[@.7+x]-f[@.2+x]-f[@.7+x]g[@.7+x]-f[@.2+x]-f[@.7+x]
}

ADH06 { ; Anthony Hanmer 1999
   Angle 8
   Axiom +X

   X=f[@.9x]-f[@.1x]-f[@.9x]f[@.9x]-f[@.1x]-f[@.9x]f[@.9x]-f[@.1x]-f[@.9x]f[@.9x]-f[@.1x]-f[@.9x]
}

ADH12b { ; Anthony Hanmer 1999
   Angle 6
   Axiom X

   X=f[@.7+x]--f[@.2+x]--f[@.7+x]f[@.7+x]--f[@.2+x]--f[@.7+x]f[@.7+x]--f[@.2+x]--f[@.7+x]f[@.7+x]--f[@.2+x]--f[@.7+x]
}

ADH13a { ; Anthony Hanmer 1999
   Angle 3
   Axiom X

   X=f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]f[@.7+x]-f[@.2+x]-f[@.7+x]
}

ADH16a { ; Anthony Hanmer 1999
   Angle 5
   Axiom +X

   X=f[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]
}

ADH16b { ; Anthony Hanmer 1999
   Angle 4
   Axiom +X

   X=f[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]
}

ADH16c { ; Anthony Hanmer 1999
   Angle 4
   Axiom +X

   X=f[@q7fx]-[@q2fx]-[@q7fx]+[@q7fx]-[@q2fx]-[@q7fx]+[@q7fx]-[@q2fx]-[@q7fx]+[@q7fx]-[@q2fx]-[@q7fx]
}

ADH17 { ; Anthony Hanmer 1999
   Angle 4
   Axiom X

   X=f[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]
}

ADH17a { ; Anthony Hanmer 1999
   Angle 4
   Axiom X

   X=f[@.6fx]-[@.2fx]-[@.6fx]+[@.6fx]-[@.2fx]-[@.6fx]+[@.6fx]-[@.2fx]-[@.6fx]+[@.6fx]-[@.2fx]-[@.6fx]
}

ADH17d { ; Anthony Hanmer 1999
   Angle 4
   Axiom X

   X=f[@q6fx]-[@.2fx]-[@q6fx]+[@q6fx]-[@.2fx]-[@q6fx]+[@q6fx]-[@.2fx]-[@q6fx]+[@q6fx]-[@.2fx]-[@q6fx]
}

ADH22 { ; Anthony Hanmer 1999
   Angle 3
   Axiom +X

   X=f[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]
}

ADH22b { ; Anthony Hanmer 1999
   Angle 3
   Axiom +X

   X=f[@.7fx]-[@.3fx]-[@.7fx]g[@.7fx]-[@.3fx]-[@.7fx]g[@.7fx]-[@.3fx]-[@.7fx]g[@.7fx]-[@.3fx]-[@.7fx]
}

ADH22d { ; Anthony Hanmer 1999
   Angle 3
   Axiom +X

   X=-[@.3fx]-[@.7fx]+[@.7fx]-[@.3fx]-[@.7fx]+[@.7fx]-[@.3fx]-[@.7fx]+[@.7fx]-[@.3fx]-[@.7fx]
}

ADH23 { ; Anthony Hanmer 1999
   Angle 3
   Axiom X

   X=f[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]+[@.9fx]-[@.1fx]-[@.9fx]
}

ADH24 { ; Anthony Hanmer 1999
   Angle 3
   Axiom +X

   X=f[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]
}

ADH27 { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]+[@.7fx]-[@.2fx]-[@.7fx]
}

ADH28 { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.6fx]-[@.3fx]-[@.6fx]+[@.6fx]-[@.3fx]-[@.6fx]+[@.6fx]-[@.3fx]-[@.6fx]+[@.6fx]-[@.3fx]-[@.6fx]+[@.6fx]-[@.3fx]-[@.6fx]+[@.6fx]-[@.3fx]-[@.6fx]
}

ADH28a { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]+[@.5fx]-[@.4fx]-[@.5fx]+[@.5fx]-[@.4fx]-[@.5fx]+[@.5fx]-[@.4fx]-[@.5fx]+[@.5fx]-[@.4fx]-[@.5fx]+[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28b { ; Anthony Hanmer 1999
   Angle 6 ; slow but interesting
   Axiom +X

   X=f[@.5fx]--[@.4fx]--[@.5fx]+[@.5fx]--[@.4fx]--[@.5fx]+[@.5fx]--[@.4fx]--[@.5fx]+[@.5fx]--[@.4fx]--[@.5fx]+[@.5fx]--[@.4fx]--[@.5fx]+[@.5fx]--[@.4fx]--[@.5fx]
}

ADH28e { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]
}

ADH28f { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28i { ; Anthony Hanmer 1999
   Angle 5
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28m { ; Anthony Hanmer 1999
   Angle 8
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28o { ; Anthony Hanmer 1999
   Angle 10
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28p { ; Anthony Hanmer 1999
   Angle 12
   Axiom +X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH28s { ; Anthony Hanmer 1999
   Angle 36
   Axiom +++++++++X

   X=f[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]g[@.5fx]-[@.4fx]-[@.5fx]
}

ADH32a { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]+[@.5fx]g[@.4fx]g[@.5fx]
}

ADH33a { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]g[@.3fx]g[@.5fx]+[@.5fx]g[@.3fx]g[@.5fx]+[@.5fx]g[@.3fx]g[@.5fx]+[@.5fx]g[@.3fx]g[@.5fx]+[@.5fx]g[@.3fx]g[@.5fx]+[@.5fx]g[@.3fx]g[@.5fx]
}

ADH34a { ; Anthony Hanmer 1999
   Angle 6
   Axiom +X

   X=f[@.5fx]g[@I2fx]g[@.5fx]+[@.5fx]g[@I2fx]g[@.5fx]+[@.5fx]g[@I2fx]g[@.5fx]+[@.5fx]g[@I2fx]g[@.5fx]+[@.5fx]g[@I2fx]g[@.5fx]+[@.5fx]g[@I2fx]g[@.5fx]
}

ADH59 { ; Anthony Hanmer 1999
   Angle 6
   Axiom +x
   x=[@.48f+x+x+x+x]
}

ADH60 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x
   x=[@.6f+x+x+x+x]
}

ADH68 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx-x++x-x
}

ADH70 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x-x-x-x-x-x
   x=fx-x++x+x-x-x
}

ADH70c { ; Anthony Hanmer 1999
   Angle 6
   Axiom xx-xx-xx-xx-xx-xx
   x=fx-x++x+x-x-x
}

ADH71 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x-x-x-x-x-x
   x=fx-x++x-x
}

ADH72 { ; Anthony Hanmer 1999
   Angle 8
   Axiom x-x-x-x-x-x-x-x
   x=fx+x--x+x--x+x--x+x
}

ADH72b { ; Anthony Hanmer 1999
   Angle 8
   Axiom xx-xx-xx-xx-xx-xx-xx-xx
   x=fx+x--x+x--x+x--x+x
}

ADH72c { ; Anthony Hanmer 1999
   Angle 8
   Axiom xx+xx+xx+xx+xx+xx+xx+xx
   x=fx+x--x+x--x+x--x+x
}

ADH73b { ; Anthony Hanmer 1999
   Angle 8
   Axiom xx-xx-xx-xx-xx-xx-xx-xx
   x=fx-x++x-x++x-x++x
}

ADH73c { ; Anthony Hanmer 1999
   Angle 8
   Axiom xx+xx+xx+xx+xx+xx+xx+xx
   x=fx-x++x-x++x-x++x
}

ADH75 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx-x+x+x-x
}

ADH75a { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx-gxf+x+gxf-x
}

ADH76a { ; Anthony Hanmer 1999
   Angle 3
   Axiom x-x-x
   x=fx-x+x
}

ADH76d { ; Anthony Hanmer 1999
   Angle 3
   Axiom x+x+x
   x=fx-x+x
}

ADH76f { ; Anthony Hanmer 1999
   Angle 3
   Axiom xx+xx+xx
   x=fx-x+x
}

ADH78 { ; Anthony Hanmer 1999
   Angle 5
   Axiom x-x-x-x-x
   x=fx-x+x+x-x
}

ADH78a { ; Anthony Hanmer 1999
   Angle 5
   Axiom x+x+x+x+x
   x=fx-x+x+x-x
}

ADH78b { ; Anthony Hanmer 1999
   Angle 5
   Axiom xx-xx-xx-xx-xx
   x=fx-x+x+x-x
}

ADH78c { ; Anthony Hanmer 1999
   Angle 5
   Axiom xx+xx+xx+xx+xx
   x=fx-x+x+x-x
}

ADH79 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x-x-x-x-x-x
   x=fx--x+x+x-x+x
}

ADH79b { ; Anthony Hanmer 1999
   Angle 6
   Axiom xx-xx-xx-xx-xx-xx
   x=fx--x+x+x-x+x
}

ADH79c { ; Anthony Hanmer 1999
   Angle 6
   Axiom xx+xx+xx+xx+xx+xx
   x=fx--x+x+x-x+x
}

ADH80 { ; Anthony Hanmer 1999
   Angle 6 ; Snowflake variation 1
   Axiom x-x-x-x-x-x
   x=fx-x++x-x+x-x
}

ADH80a { ; Anthony Hanmer 1999
   Angle 6 ; Snowflake variation 2
   Axiom x+x+x+x+x+x
   x=fx-x++x-x+x-x
}

ADH80b { ; Anthony Hanmer 1999
   Angle 6  ; Snowflake variation 3
   Axiom xx-xx-xx-xx-xx-xx
   x=fx-x++x-x+x-x
}

ADH80c { ; Anthony Hanmer 1999
   Angle 6  ; Snowflake variation 4
   Axiom xx+xx+xx+xx+xx+xx
   x=fx-x++x-x+x-x
}

ADH81 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx-x+x-x+x
}

ADH81a { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   x=fx-x+x-x+x
}

ADH81b { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx-xx-xx-xx
   x=fx-x+x-x+x
}

ADH81c { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx+xx+xx+xx
   x=fx-x+x-x+x
}

ADH82 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx--x-x-x+x
}

ADH82a { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   x=fx--x-x-x+x
}

ADH82b { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx-xx-xx-xx
   x=fx--x-x-x+x
}

ADH82c { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx+xx+xx+xx
   x=fx--x-x-x+x
}

ADH83 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x-x-x-x
   x=fx-x--x-x+x
}

ADH83a { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   x=fx-x--x-x+x
}

ADH83b { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx-xx-xx-xx
   x=fx-x--x-x+x
}

ADH83c { ; Anthony Hanmer 1999
   Angle 4
   Axiom xx+xx+xx+xx
   x=fx-x--x-x+x
}

ADH84a { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   x=f-x++x-x++x
}

ADH85 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   x=fx-x+x+x-x
}

ADH86 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x+x+x+x+x+x
   x=fx-x++x+x-x-x
}

ADH87 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x-x-x-x-x-x
   X=f[@.6fx]-[@I2fx]-[@.6fx]+[@.6fx]-[@I2fx]-[@.6fx]+[@.6fx]-[@I2fx]
}

ADH93 { ; Anthony Hanmer 1999
   Angle 6
   Axiom x+x+x+x+x+x
   X=f[@.6fx]--[@I2fx]--[@.6fx]+[@.6fx]--[@I2fx]--[@.6fx]+[@.6fx]--[@I2fx]
}

ADH94 { ; Anthony Hanmer 1999
   Angle 4
   Axiom x+x+x+x
   X=f[@.6fx]-[@I2fx]-[@.6fx]+[@.6fx]-[@I2fx]-[@.6fx]+[@.6fx]-[@I2fx]
}

ADH99 { ; Anthony Hanmer 1999
   Angle 4
   Axiom xy-xy-xy-xy
   x=fx-fyx+fyx+x-y
   y=fy-fxy+fxy+y-x
}

ADH99d { ; Anthony Hanmer 1999
   Angle 4
   Axiom xy+xy+xy+xy
   x=fx-fyx+fyx+x
   y=fy-fxy+fxy+y
}

ADH99f { ; Anthony Hanmer 1999
   Angle 4
   Axiom xy-xy-xy-xy
   x=fx-fyx+fyx
   y=fy-fxy+fxy
}

ADH99h { ; Anthony Hanmer 1999
   Angle 4
   Axiom xy-xy-xy-xy
   x=-fyx+gxfy
   y=+fxy-gyfx
}

; Anthony David Hanmer
; (Anagramatically known as:
; Ah! handy, mad inventor)
;
; Baku, Azerbaijan
