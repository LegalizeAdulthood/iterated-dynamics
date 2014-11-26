comment {
  This file is part of the FRMTUT.ZIP package.  It is a compilation of the
  formulas discussed in FRMTUTOR.TXT revision 1.0, and is included so that
  the reader can see how the formulas work.  Please refer to FRMTUTOR.TXT
  for more information.
}


Mandelbrot (xaxis) { ;The classic Mandelbrot set
  z = 0, c = pixel:
    z = z*z + c
    |z| < 4
}


frm-A (xaxis) { ;Another formula for the Mandelbrot set
  z = const = pixel:
    z = z^2 + const
    |z| < 4
}


frm-B { ;A generalized Julia formula
        ;For the traditional Julia algorithm, set FN1() to SQR,
        ;and then try different values for P1
  z = pixel:
    z = fn1(z) + p1
    |z| <= (4 + p2)
}


frm-C1 {
  z = 0
  c = pixel:
    z = sqr(z) + c
    |z| < 4
}


frm-C2 { z = 0, c = pixel: z = sqr(z) + c, |z| < 4 }


Cardioid { ;author not listed
  z = 0, x = real(pixel), y=imag(pixel),
  c=x*(cos(y)+x*sin(y)):
  z=sqr(z)+c,
  |z| < 4
}


CGNewtonSinExp (XAXIS) { ;by Chris Green
  ; Use floating point, and set P1 to some positive value.
  z=pixel:
  z1=exp(z),
  z2=sin(z)+z1-z,
  z=z-p1*z2/(cos(z)+z1),
  .0001 < |z2|
}


Mutantbrot { ;A mutation of the classic Mandelbrot set
  z = 0, c = pixel:      ;standard initialization section
    z = z*z + c + sin(z) ;mutated iterated section
    |z| < 4              ;standard bailout test
}


speed-A { ;Demonstrates potential for speed-up
  z = 0:
    z = z*z + sin(pixel)
    |z| < 4
}


speed-B { ;variation of speed-A showing one speed-up technique
  z = 0, sinp = sin(pixel):
    z = z*z + sinp
    |z| < 4
}


IfThen-A1 { ;Demonstrates that the order of expressions can make a
          ;difference.  In this example, the assignment is performed
          ;BEFORE the comparison.
  z = c = pixel:
    (z < 0) * (z = fn1(z) + c)
    (0 <= z) * (z = fn2(z) + c)
    |z| < 4
}


IfThen-A2 { ;Functional equivalent of IfThen-A2
  z = c = pixel:
    z = fn1(z) + c
    z = fn2(z) + c
    |z| < 4
}


IfThen-A3 { ;Another equivalent of IfThen-A1
  z = c = pixel:
    z = fn2(fn1(z) + c) + c
    |z| < 4
}


IfThen-B1 { ;In this formula, the comparison is performed BEFORE the
          ;assignment, but there's still a subtle flaw.
  z = c = pixel:
    (z = fn1(z) + c) * (z < 0)
    (z = fn2(z) + c) * (0 <= z)
    |z| < 4
}


IfThen-B2 { ;Functional equivalent of IfThen-B1
  z = c = pixel:
    z = (fn1(z) + c) * (z < 0)
    z = (fn2(z) + c) * (0 <= z)
    |z| < 4
}


IfThen-C1 { ;What we REALLY had in mind.
  z = c = pixel:
    neg = fn1(z) * (z < 0)
    pos = fn2(z) * (0 <= z)
    z = neg + pos + c
    |z| < 4
}


IfThen-C2 { ;An alternate version of IfThen-C1
  z = c = pixel:
    z = (fn1(z) * (z < 0)) + (fn2(z) * (0 <= z)) + c
    |z| < 4
}


bailout-A { ;Hard coded bailout value
  ;p1 = parameter (default 0,0)
  z = pixel, c = fn1(pixel):
    z = fn2(z*z) + c + p1
    |z| < 4
}


bailout-B { ;Variable default -- additive
  ;p1 = parameter (default 0,0)
  ;p2 = bailout adjustment value (default 0,0)
  test = (4 + p2)
  z = pixel, c = fn1(pixel):
    z = fn2(z*z) + c + p1
    |z| < test
}


bailout-C { ;Variable default -- conditional logic
  ;This formula requires floating-point
  ;p1 = parameter (default 0,0)
  ;p2 = bailout   (default 4,0)
  ;The following line sets test = 4 if real(p2) = 0, else test = p2
  test = (4 * (p2 <= 0)) + (p2 * (0 < p2))
  z = pixel, c = fn1(pixel):
    z = fn2(z*z) + c + p1
    |z| < test
}


fibo-A { ;Fibonacci / Mandelbrot hybrid
  z = oldz = c = pixel:
    temp = z
    z = z * oldz + c
    oldz = temp
    |z| < 4
}


fibo-B { ;Mutation of fibo-A
  z = oldz = c = pixel:
    temp = z
    z = fn1(z * oldz) + c
    oldz = temp
    |z| < 4
}


inandout01 { ;Bradley Beacham  [74223,2745]
  ;p1 = Parameter (default 0), real(p2) = Bailout (default 4)
  ;The next line sets test=4 if real(p2)<=0, else test=real(p2)
  test = (4 * (real(p2)<=0) + real(p2) * (0<p2))
  z = oldz = pixel, c1 = fn1(pixel), c2 = fn2(pixel):
    a = (|z| <= |oldz|) * (c1) ;IN
    b = (|oldz| < |z|)  * (c2) ;OUT
    oldz = z
    z = fn3(z*z) + a + b + p1
    |z| <= test
}


dissected-A { ;A dissected Mandelbrot
  z = 0, c = pixel:
    x = real(z), y = imag(z)   ;isolate real and imaginary parts
    newx = x*x - y*y           ;calculate real part of z*z
    newy = 2*x*y               ;calculate imag part of z*z
    z =  newx + flip(newy) + c ;reassemble z
    |z| < 4
}


dissected-B { ;A mutation of "dissected-A"
  z = 0, c = pixel, k = 2 + p1:
    x = real(z), y = imag(z)
    newx = fn1(x*x) - fn2(y*y)
    newy = k*fn3(x*y)
    z =  newx + flip(newy) + c
    |z| < 4
}


shifter { ;Use a counter to shift algorithms
  z = c = pixel, iter = 1, shift = p1, test = 4 + p2:
    lo = (z*z) * (iter <= shift)
    hi = (z*z*z) * (shift < iter)
    iter = iter + 1
    z = lo + hi + c
    |z| < test
}


sym-A { ;Non-symmetrical fractal
  z = c = pixel, k = (2.5,0.5):
    z = z^k + c
    |z| < 4
}


sym-B (xaxis) { ;Sym-A with symmetry declared in error
  z = c = pixel, k = (2.5,0.5):
    z = z^k + c
    |z| < 4
}


frm-D1 { ;Unparsable expression ignored
  z = c = pixel:
    z = z*z + sin z + c
    |z| < 4
}


frm-D2 { ;fixed version of frm-D1
  z = c = pixel:
    z = z*z + sin(z) + c
    |z| < 4
}


weirdo { ;Mandelbrot with no bailout test
  z = c = pixel:
  z = z*z + c
}

ghost { ;Demonstrates strange parser behavior
        ;To see effect, use floating point and make sure
        ;FN2() is not IDENT
  z = oldz = c1 = pixel, c2 = fn1(pixel)
  tgt = fn2(pixel), rt = real(tgt), it = imag(tgt):
    oldx = real(oldz) - rt
    oldy = imag(oldz) - it
    olddist = (oldx * oldx) + (oldy * oldy)
    x = real(z) - rt
    y = imag(z) - it
    dist = (x * x) + (y * y)
    a = (dist <= olddist) * (c1)
    b = (olddist < dist)  * (c2)
    oldz = z
    z = z*z + a + b
    |z| <= 4
}

ghostless-A { ;One solution to the ghost problem  -- reorder expressions
  z = oldz = c1 = pixel, c2 = fn1(pixel)
  tgt = fn2(pixel), rt = real(tgt), it = imag(tgt):
    oldx = real(oldz) - rt
    oldy = imag(oldz) - it
    olddist = (oldx * oldx) + (oldy * oldy)
    x = real(z) - rt
    y = imag(z) - it
    dist = (x * x) + (y * y)
    a = (c1) * (dist <= olddist) ;Reverse order of value and comparison
    b = (c2) * (olddist < dist)  ;Ditto
    oldz = z
    z = z*z + a + b
    |z| <= 4
}

ghostless-B { ;Another solution to the ghost problem -- reinitialize
  z = oldz = c1 = pixel, c2 = fn1(pixel)
  tgt = fn2(pixel), rt = real(tgt), it = imag(tgt):
    oldx = real(oldz) - rt
    oldy = imag(oldz) - it
    olddist = (oldx * oldx) + (oldy * oldy)
    x = real(z) - rt
    y = imag(z) - it
    dist = (x * x) + (y * y)
    a = b = 0                    ;Make sure a & b are set to zero
    a = (dist <= olddist) * (c1)
    b = (olddist < dist) * (c2)
    oldz = z
    z = z*z + a + b
    |z| <= 4
}


ghostless-C { ;Yet another solution -- simplify!
  z = c1 = pixel, c2 = fn1(pixel), olddist = 100
  tgt = fn2(pixel), rt = real(tgt), it = imag(tgt):
    x = real(z) - rt
    y = imag(z) - it
    dist = (x * x) + (y * y)
    a = (dist <= olddist) * (c1)
    b = (olddist < dist)  * (c2)
    olddist = dist
    z = z*z + a + b
    |z| <= 4
}

