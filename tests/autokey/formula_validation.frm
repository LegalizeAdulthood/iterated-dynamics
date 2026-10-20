; SPDX-License-Identifier: GPL-3.0-only
; Copyright 2026 Richard Thomson.
;
bad {
  z = *pixel
}
empty {
}
good {
  z = pixel:
  z = sqr(z) + pixel
  |z| <= 4
}
