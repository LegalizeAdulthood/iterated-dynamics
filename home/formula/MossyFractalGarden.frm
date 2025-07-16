MandelbrotBC3   { ; by several Fractint users
  e = p1
  a = imag(p2) + 100
  p = real(p2) + pi
  pi2 = 2 * pi
  q = 2 * pi * fn1(p / pi2)
  r = real(p2) + pi - q
  fq = flip(q)
  fpi2 = flip(pi2)
  z = c = pixel:
    z = log(z)
    if (imag(z) > r)
      z = z + fpi2
    endif
    z = exp(e * (z + fq)) + c
  |z| < a
}
