; SPDX-License-Identifier: GPL-3.0-only
; Copyright 2026 Richard Thomson.
;
bad {
  axiom f
  angle 2
}
good {
  axiom f+f+f+f
  angle 4
  f = f+f-f-f+f
}
