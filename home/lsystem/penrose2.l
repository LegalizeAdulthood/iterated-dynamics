; SPDX-License-Identifier: GPL-3.0-only
;
PenroseDouble { ; by Herb Savage
   ; This is Penrose3 and Penrose4 superimposed
   Angle 10
   Axiom [X][Y]++[X][Y]++[X][Y]++[X][Y]++[X][Y]
   W=YF++ZF----XF[-YF----WF]++
   X=+YF--ZF[---WF--XF]+
   Y=-WF++XF[+++YF++ZF]-
   Z=--YF++++WF[+ZF++++XF]--XF
   F=
}

; Generation of Penrose aperiodic tilings
;
; Thanks to Herb Savage, who showed it was possible to generate those tilings
; with L-systems.
;
; Based on decomposition of tilings described in Tilings and Patterns, by
; Branko Grunbaum and G.C. Shepard, W.H. Freeman and Company, chapter 10
;
; Philippe Hurbain
