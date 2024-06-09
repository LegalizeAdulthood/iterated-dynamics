; Mandelbrot Derivatives
;
; Putting abs() into various parts of the Mandelbrot formula 
; generates a whole new range of beautiful fractals.  The most
; famous is "Burning Ship".  Many others have since been discovered.
; here are a few.
;
; Paul de Leeuw - Thursday, 23rd of May 2024.
;

Mandelbrot { ; Good old Mandelbrot - added for completeness
   z = c = pixel:
   z = sqr(z) + pixel + p1,
   |z| < 4
}
  
Burning_Ship { ; Burning Ship - the original Mandelbrot Derivative
   z = c = pixel:
   x = abs(real(z))
   y = -abs(imag(z))
   z = x + flip(y)
   z = z*z + pixel + p1,
   |z| < 4
}
  
Perp_Mandelbrot { ; Perpendicular Mandelbrot
   z = c = pixel:
   x = abs(real(z))
   y = -imag(z)
   z = x + flip(y)
   z = z*z + pixel + p1,
   |z| < 4
}

Perp_Burning_Ship  { ; Perpendicular Burning Ship
   z = c = pixel:
   x = real(z)
   y = -abs(imag(z))
   z = x + flip(y)
   z = z*z + pixel + p1,
   |z| < 4
}
  
Heart_Mandelbrot  { ; Heart Mandelbrot
   z = c = pixel:
   x = abs(real(z))
   y = imag(z)
   z = x + flip(y)
   z = z*z + pixel + p1,
   |z| < 4
}
  
Mandelbar { ; Mandelbar
   z = c = pixel:
   x = -real(z)
   y = imag(z)
   z = x + flip(y)
   z = z*z + pixel + p1,
   |z| < 4
}

Celtic_Mandelbrot  { ; Celtic Mandelbrot
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = flip(x*y*2.0) + abs(x*x - y*y)
   z = z + pixel + p1,
   |z| < 4
}
  
Celtic_Mandelbar { ; Celtic Mandelbar
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = flip(-x*y*2.0) + abs(x*x - y*y)
   z = z + pixel + p1,
   |z| < 4
}

Perp_Celtic { ; Perpendicular Celtic
   z = c = pixel:
   x = abs(real(z))
   y = imag(z)
   z = flip(-x*y*2.0) + abs(x*x - y*y)
   z = z + pixel + p1,
   |z| < 4
}

Celtic_Heart { ; Celtic Heart
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = abs(x*x - y*y) + flip(abs(x)*y*2.0)
   z = z + pixel + p1,
   |z| < 4
}

Buffalo { ; Buffalo
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = flip(-abs(x*y*2.0)) + abs(x*x - y*y)
   z = z + pixel + p1,
   |z| < 4
}

Perp_Buffalo { ; Perpendicular Buffalo
   z = c = pixel:
   x = real(z)
   y=imag(z)
   z = flip(-x*abs(y)*2.0) + abs(x*x - y*y)
   z = z + pixel + p1,
   |z| < 4
}

Cubic_MandelbrOrth { ; Cubic Mandelbar Orthogonal
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = -(x*x - (y*y*3.0))*x + flip(((x*x*3.0) - y*y)*y) + pixel + p1,
   |z| < 4
}

Cubic_MandelbrDiag { ; Cubic Mandelbar Diagonal
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*x + flip(-((x*x*3.0) - y*y)*y) + pixel + p1,
   |z| < 4
}

Cubic_Burning_Ship { ; Cubic Burning Ship
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*abs(x) + flip(((x*x*3.0) - y*y)*abs(y)) + pixel + p1,
   |z| < 4
}

Cubic_Buffalo { ; Cubic Buffalo
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = abs((x*x - (y*y*3.0))*x) + flip(abs(((x*x*3.0) - y*y)*y)) + pixel + p1,
   |z| < 4
}

Cubic_Quasi_Heart { ; Cubic Quasi Heart
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*abs(x) + flip(abs(((x*x*3.0) - y*y))*y) + pixel + p1,
   |z| < 4
}

Cubic_Quasi_Perp { ; Cubic Quasi Perpendicular
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*abs(x) - flip(abs(((x*x*3.0) - y*y))*y) + pixel + p1,
   |z| < 4
}

Cubic_QuasiBurnShp { ; Cubic Quasi Burning Ship (Hybrid)
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*abs(x) - flip(abs(((x*x*3.0) - y*y)*y)) + pixel + p1,
   |z| < 4
}

Part_Cub_Buff_Real { ; Partial Cubic Buffalo Real (Celtic)
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = abs((x*x - (y*y*3.0))*x) + flip(((x*x*3.0) - y*y)*y) + pixel + p1,
   |z| < 4
}

Part_Cub_Buff_Imag { ; Partial Cubic Buffalo Imaginary
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*x + flip(abs(((x*x*3.0) - y*y)*y)) + pixel + p1,
   |z| < 4
}

Part_Cub_BS_Real { ; Partial Cubic Burning Ship Real
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*abs(x) + flip(((x*x*3.0) - y*y)*y) + pixel + p1,
   |z| < 4
}

Part_Cub_BS_Imag { ; Partial Cubic Burning Ship Imaginary
   z = c = pixel:
   x = real(z)
   y = imag(z)
   z = (x*x - (y*y*3.0))*x + flip(((x*x*3.0) - y*y)*abs(y)) + pixel + p1,
   |z| < 4
}
