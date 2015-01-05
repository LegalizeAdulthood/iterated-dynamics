   {=========================================================================
   =        File originally distributed with FRAC'Cetera Vol 2 Iss 7        =
   ==========================================================================
   =  Original idea submitted to FRAC'Cetera by : Ray Girvan.  Generalized  =
   =  by Jon Horner.
   =  F'names, where present, represent FRAC'Cetera created variations or   =
   =  derivatives based, often quite loosely, on the author's originals.    =
   =========================================================================}

!_Press_F2_!     {; There is text in this formula file.  Shell to DOS with
                  ; the <d> key and use a text reader to browse the file.
                 }

Julitile1               { ; Based on an idea by Ray Girvan
           z = fn1(real(pixel))+p1*fn2(imag(pixel)) :
           z = z * z + p2
           |z| <=4 }

Julitile2XY(XYaxis)     { ; force x/y symmetry - Jon Horner
           z = (0,0),
           z = fn1(real(pixel))+p1*fn2(imag(pixel)) :
           z = z * z + p2
           |real(z)| <=10 || |imag(z)| <=10  }

Julitile3b              { ; No sym - Jon Horner
           z = fn1(real(pixel))+p1*fn2(imag(pixel)) :
           z = z * z + p2
           |real(z)| <=10 || |imag(z)| <=10    }

Julitile3c              { ; Jon Horner
           z = fn1(real(pixel))+p1*fn2(imag(pixel)), c = pixel :
           z = z * z + c
           |real(z)| <=10 || |imag(z)| <=10    }

Julitile3d              { ; Jon Horner
           z = c = pixel,
           z = fn1(real(pixel)) + c*fn2(imag(pixel)),
           test = p1 + 10 :
           z = z * z + p2
           |real(z)| <=test || |imag(z)| <=test    }

Julitile3e              { ; Jon Horner
           z = c = pixel,
           z = fn1(real(pixel)) + c*fn2(imag(pixel)) :
           z = z ^ p1 + p2
           |real(z)| <=10   || |imag(z)| <=10      }


