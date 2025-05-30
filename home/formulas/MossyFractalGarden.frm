; GitHub Issue #17: renders incorrectly
MandelbrotBC3   { ; by several Fractint users
  e=p1, a=imag(p2)+100
  p=real(p2)+PI
  q=2*PI*fn1(p/(2*PI))
  r=real(p2)+PI-q
  Z=C=Pixel:
    Z=log(Z)
    IF(imag(Z)>r)
      Z=Z+flip(2*PI)
    ENDIF
    Z=exp(e*(Z+flip(q)))+C
  |Z|<a }

; GitHub Issue #17: renders correctly
MandelbrotBC3-2   { ; by several Fractint users
  e =p1, a=imag(p2)+100
  p=real(p2)+PI
  q=p
  r=0
  c2=flip(2*PI)
  fq = flip(q)
  Z=C=Pixel:
    Z=log(Z)
    IF(imag(Z)>r)
      Z=Z+c2
    ENDIF
    Z=exp(e*(Z+fq))+C
  |Z|<a }
