comment {
                  Formula file provided with PHCTUTOR.TXT
                  ---------------------------------------
                      All formulas by Sylvie Gallet
                      -----------------------------
                 CompuServe: Sylvie_Gallet  [101324,3444]
                   Internet: sylvie_gallet@compuserve.com
  }

mandel { ; Mandel set of z^2 + c
  z = c = pixel :
   z = z*z + c
    |z| <= 4
  }

julia { ; Julia set of z^2 + (-0.75,0.1234)
  z = pixel , c = (-0.75,0.1234) :
   z = z*z + c
    |z| <= 4
  }

newton { ; Julia set of Newton's method applied to z^3 - 1 = 0
  z = pixel :
   n = z^3 - 1 , d = 3*z^2
   z = z - n/d
    |n| >= 0.000001
  }

; phc_mj { ; overlay the Mandel set of z^2 + c with
;         ; the Julia set of z^2 + (-0.75,0.1234)
;  z = pixel
;  c = pixel * (whitesq == 0) + (-0.75,0.1234) * whitesq :
;   z = z*z + c
;    |z| <= 4
;  }

phc_mj { ; overlay the Mandel set of z^2 + c with
         ; the Julia set of z^2 + (-0.75,0.1234)
         ; Modified for if..else logic, April 1997
   z = pixel
   IF (whitesq)
      c = (-0.75,0.1234)
   ELSE
      c = pixel
   ENDIF
   :
   z = z*z + c
   |z| <= 4
   }

: phc_mn_A { ; overlay the Mandel set of z^2 + c with the Julia
;           ; set of Newton's method applied to z^3 - 1 = 0
;  z = c = pixel :
;   n = z^3 - 1 , d = 3*z^2
;   z = (z*z + c) * (whitesq == 0) + (z - n/d) * whitesq
;    (|z| <= 4 && whitesq == 0) || (|n| >= 0.000001 && whitesq)
;  }

phc_mn_A { ; overlay the Mandel set of z^2 + c with the Julia
           ; set of Newton's method applied to z^3 - 1 = 0
           ; Modified for if..else logic, April 1997
   z = pixel :
   IF (whitesq)
      n = z^3 - 1 , d = 3*z^2 , z = z - n/d
      PHC_bailout = |n| >= 0.000001
   ELSE
      z = z*z + pixel , PHC_bailout = |z| <= 4
   ENDIF
   PHC_bailout
   }

; ptc_mjn_A { ; overlay the Mandel set of z^2 + c with the Julia set
;            ; of z^2 + (-0.75,0.1234) and the Julia set of Newton's
;            ; method applied to z^3 - 1 = 0
;  cr = real(scrnpix) + imag(scrnpix)
;  r = cr - 3 * trunc(cr / real(3))
;  z = pixel
;  c = pixel * (r == 0) + (-0.75,0.1234) * (r == 1) :
;   n = z^3 - 1 , d = 3*z^2
;   z = (z*z + c) * (r == 0 || r == 1) + (z - n/d) * (r == 2)
;    (|z| <= 4 && (r == 0 || r == 1)) || (|n| >= 0.000001 && r == 2)
;  }

ptc_mjn_A { ; overlay the Mandel set of z^2 + c with the Julia set
            ; of z^2 + (-0.75,0.1234) and the Julia set of Newton's
            ; method applied to z^3 - 1 = 0
            ; Modified for if..else logic, April 1997
   cr = real(scrnpix) + imag(scrnpix)
   r = cr - 3 * trunc(cr / real(3))
   z = pixel
   IF (r == 0)
      c = pixel
   ELSEIF (r == 1)
      c = (-0.75,0.1234)
   ENDIF
   :
   IF (r == 2)
      n = z^3 - 1 , d = 3*z^2 , z = z - n/d
      PTC_bailout = |n| >= 0.000001
   ELSE
      z = z*z + c
      PTC_bailout = |z| <= 4
   ENDIF
   PTC_bailout
   }

mand_0 {
  z = c = sin(pixel) :
   z = z*z + c
    |real(z)| <= 4
  }

mand_1 {
  z = c = pixel :
   z = z*z + c
    |z| <= 4
  }

mand_2 {
  z = c = 1/pixel :
   z = z*z + c
    |imag(z)| <= 4
  }

mand_3 {
  z = c = -pixel :
   z = z*z + c
    |real(z)+imag(z)| <= 4
  }

; phc_4m_A { ; overlay four Mandels with different initializations
;           ; and bailout tests
;           ; Isn't it terrific???
;  cr = real(scrnpix) + 2*imag(scrnpix)
;  r = cr - 4 * trunc(cr / 4)
;  c0 = sin(pixel) * (r == 0)
;  c1 = pixel * (r == 1)
;  c2 = 1/pixel * (r == 2)
;  c3 = -pixel * (r == 3)
;  z = c = c0 + c1 + c2 + c3 :
;   z = z*z + c
;   test_0 = |real(z)| <= 4  &&  r == 0
;   test_1 = |z| <= 4  &&  r == 1
;   test_2 = |imag(z)| <= 4  &&  r == 2
;   test_3 = |real(z)+imag(z)| <= 4  &&  r == 3
;    test_0 || test_1 || test_2 || test_3
;  }

ptc_4m_A { ; overlay four Mandels with different initializations
           ; and bailout tests
           ; Isn't it terrific???
           ; Modified for if..else logic, April 1997
   cr = real(scrnpix) + 2 * imag(scrnpix)
   r = cr - 4 * trunc(cr / 4)
   IF (r == 0)
      z = c = sin(pixel)
   ELSEIF (r == 1)
      z = c = pixel
   ELSEIF (r == 2)
      z = c = 1/pixel
   ELSE
      z = c = -pixel
   ENDIF
   :
   z = z*z + c
   IF (r == 0)
      PTC_bailout = |real(z)| <= 4
   ELSEIF (r == 1)
      PTC_bailout = |z| <= 4
   ELSEIF (r == 2)
      PTC_bailout = |imag(z)| <= 4
   ELSE
      PTC_bailout = |real(z)+imag(z)| <= 4
   ENDIF
   PTC_bailout
   }

newton_B { ; Julia set of Newton's method applied to z^3 - 1 = 0
  z = pixel :
   z2 = z*z , n = z2*z - 1 , d = 3*z2
   z = z - n/d
    |n| >= 0.000001
  }

; phc_mn_B { ; overlay the Mandel set of z^2 + c with the Julia
;           ; set of Newton's method applied to z^3 - 1 = 0
;  z = c = pixel :
;   z2 = z*z , n = z2*z - 1 , d = 3*z2
;   z = (z2 + c) * (whitesq == 0) + (z - n/d) * whitesq
;    (|z| <= 4 && whitesq == 0) || (|n| >= 0.000001 && whitesq)
;  }

phc_mn_B { ; overlay the Mandel set of z^2 + c with the Julia
           ; set of Newton's method applied to z^3 - 1 = 0
           ; Modified for if..else logic, April 1997
   z = pixel :
   IF (whitesq)
      z2 = z*z , n = z2*z - 1 , d = 3*z2 , z = z - n/d
      PHC_bailout = |n| >= 0.000001
   ELSE
      z = z*z + pixel , PHC_bailout = |z| <= 4
   ENDIF
   PHC_bailout
   }

; ptc_mjn_B { ; overlay the Mandel set of z^2 + c with the Julia set
;            ; of z^2 + (-0.75,0.1234) and the Julia set of Newton's
;            ; method applied to z^3 - 1 = 0
;  cr = real(scrnpix) + imag(scrnpix)
;  r = cr - 3 * trunc(cr / real(3))
;  z = pixel
;  c = pixel * (r == 0) + (-0.75,0.1234) * (r == 1) :
;  z2 = z*z , n = z2*z - 1 , d = 3*z2
;   z = (z2 + c) * (r != 2) + (z - n/d) * (r == 2)
;    (|z| <= 4 && r != 2) || (|n| >= 0.000001 && r == 2)
;  }

ptc_mjn_B { ; overlay the Mandel set of z^2 + c with the Julia set
            ; of z^2 + (-0.75,0.1234) and the Julia set of Newton's
            ; method applied to z^3 - 1 = 0
            ; Modified for if..else logic, April 1997
   cr = real(scrnpix) + imag(scrnpix)
   r = cr - 3 * trunc(cr / real(3))
   z = pixel
   IF (r == 0)
      c = pixel
   ELSEIF (r == 1)
      c = (-0.75,0.1234)
   ENDIF
   :
   IF (r == 2)
      z2 = z*z , n = z2*z - 1 , d = 3*z2 , z = z - n/d
      PTC_bailout = |n| >= 0.000001
   ELSE
      z = z*z + c
      PTC_bailout = |z| <= 4
   ENDIF
   PTC_bailout
   }

; phc_4m_B { ; overlay four Mandels with different initializations
;           ; and bailout tests
;           ; Isn't it terrific???
;  cr = real(scrnpix) + 2*imag(scrnpix)
;  r = cr - 4 * trunc(cr / 4)
;  c0 = sin(pixel) * (r == 0)
;  c2 = 1/pixel * (r == 2)
;  z = c = c0 + c2 + pixel * ((r == 1) - (r == 3)) :
;   z = z*z + c
;   test_0 = |real(z)| <= 4  &&  r == 0
;   test_1 = |z| <= 4  &&  r == 1
;   test_2 = |imag(z)| <= 4  &&  r == 2
;   test_3 = |real(z)+imag(z)| <= 4  &&  r == 3
;    test_0 || test_1 || test_2 || test_3
;  }
