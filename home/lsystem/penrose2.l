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

Kites&Darts {
              ;; by Philippe Hurbain
              ;; Penrose's kites and darts
              ;; k generates the kite, a generates the dart
  Angle 10
  Axiom k
  k=+[@.618034a]f@.618034---[-k]f-f---[-k]@i.618034f[@.618034|a]
  a=[@.618034k]+f@.618034[|a]----f+f----[a]@i.618034f
  f=g
}

Kites&Darts2 {
               ;; by Philippe Hurbain
               ;; Penrose's kites and darts, with kites seed
  Angle 10
  Axiom [k]++[k]++[k]++[k]++[k]
  k=+[@.618034a]f@.618034---[-k]f-f---[-k]@i.618034f[@.618034|a]
  a=[@.618034k]+f@.618034[|a]----f+f----[a]@i.618034f
  f=g
}

Kites&Darts3 {
               ;; by Philippe Hurbain
               ;; Penrose's kites and darts, with darts seed
  Angle 10
  Axiom [a]++[a]++[a]++[a]++[a]
  k=+[@.618034a]f@.618034---[-k]f-f---[-k]@i.618034f[@.618034|a]
  a=[@.618034k]+f@.618034[|a]----f+f----[a]@i.618034f
  f=g
}

Kites&DartsColor {
                   ;; by Philippe Hurbain
                   ;; Penrose's kites and darts, with serpentine coloring
  Angle 10
  Axiom c4[k]++[k]++[k]++[k]++[k]
  k=+[@.618034a[g---@.618c10f][--g+++@.618c10f]]
  k=f@.618034---[-k]f-f---[-k]@i.618034f[@.618034|a]
  a=[@.618034k]+f@.618034[|a][|g++@.382c10f]----f+
  a=f----[a][g--@.382c10f]@i.618034f
  f=g
}

AmmannPolyColor {
                  ;; by Philippe Hurbain
                  ;; Ammann's coloring of Penrose's rhombuses, giving an
                  ;; aperiodic tiling of 2 pentagons and 1 hexagon
  Angle 10
  Axiom c1x
  x=@.618034/36[c3\9@.66D]D[c3/196.5@.363D][/180y]\72D[/180x]
  x=[\144@.618034M@i.618034x]\108[c3\36@.509D@1.18\30D][x]D[c3\153@.66D]\72[y]D
  y=@.618034/72[x][c3\36@.509D]D\144[c3\9@.66D]D[/180y][c3/196.5@.363D]
  y=\36[y]D\144D[/180x]
  D=M
}
