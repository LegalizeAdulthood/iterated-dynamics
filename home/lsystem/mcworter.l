Alien { ; William McWorter
  Angle 11
  Axiom x
  x=[@q2@I2-fx]g[@q2@I2---fx]
  f=
}

AmmannPoly {     
     ;; by Philippe Hurbain
     ;; Same as AmmanPolyColor, showing only the
     ;; pentagon/hexagon tiling
  Angle 10
  Axiom x
  x=@.618034/36[\9@.66D]M[/196.5@.363D][/180y]\72M[/180x]
  x=[\144@.618034M@i.618034x]\108[\36@.509D@1.18\30D][x]M[\153@.66D]\72[y]M
  y=@.618034/72[x][\36@.509D]M\144[\9@.66D]M[/180y][/196.5@.363D]
  y=\36[y]M\144M[/180x]
  D=M
}

Border1 { ; William McWorter
  Angle 4
  Axiom XYXYXYX+XYXYXYX+XYXYXYX+XYXYXYX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Boundary { ; William McWorter
  ; Dekking's boundary of the Twindragon
  Angle 4
  Axiom otuz
  f=
  o=fo+f-t
  p=++f--u+f-x
  q=-f+v++f--q
  r=-f+zfs
  s=fw
  t=++f--u
  u=++f--y
  v=fs
  w=fo+f-p
  x=++f--y+f-x
  y=-f+r++f--q
  z=-f+zfw
}

Bush { ; Adrian Mariano
  Angle 16
  Axiom ++++F
  F=FF-[-F+F+F]+[+F-F-F]
}

Cross { ; William McWorter
  Angle 4
  Axiom FX
  F=
  X=FX+FX+FXFY-FY-
  Y=+FX+FXFY-FY-FY
}

Dekking { ; Dekking's Church
  ; Advances in Math, Vol. 44, 1982, pp.78-104
  Angle 4
  Axiom C32WC06ZC15YC06X
  F=
  W=FW+F-XFW-F+Z
  X=++F--Y-F+Z++F--Y-F+Z
  Y=++F--Y+F-X
  Z=FW+F-X
}

Dekking1 {
  Angle 3
  Axiom x+x+x
  x=fy+gx-fy
  y=gx+fy-gx
  f=
  g=
}

Doublecrossmed { ; William McWorter
  Angle 8
  Axiom +x
  x=x-f-y-f-x+f+y+f+x+f+y-f-x-f-y
  y=x+f+y+f+x-f-y-f-x-f-y+f+x+f+y
}

Doily { ; Adrian Mariano
  Angle 12
  Axiom f--f--f--f--f--f
  f=-f[--f--f]++f--f+
}


Doily-c {
  Angle 12
  Axiom c32f--c36f--c40f--c44f--c48f--c52f ; from Default.map
  f=-f[--f--f]++f--f+
}

Doily1 {
  Angle 12
  Axiom f--f--f--f--f--f
  f=-f[--f--f]++f[++f--f]--f+
}

Dragon { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  Angle 8
  Axiom FX
  F=
  y=+FX--FY+
  x=-FX++FY-
}

Dragon-wm { ; William McWorter
  Angle 4
  Axiom X
  F=
  X=FX+FY ; Do first half, turn left 90 degrees, do second half
  Y=FX-FY ; Trace first half (X) backwards; Y backwards is X,
          ; left turn becomes right turn, and X backwards is Y
}

Dragon1 { ; William McWorter
  Angle 8
  Axiom X
  F=
  X=FX+@.500FZ@2.00+FY
  Y=FX-@.500FZ@2.00-FY
  Z=FZ
}

DragonAlt { ; William McWorter
  Angle 8
  Axiom -c32X+[+++@.5gw---c40@q2fz]+c48y
  F=
  g=
  X=fx+[+++@.5gw---c40@q2fz]+fy
  Y=Fx-[---@.5gw+++c40@q2fz]-fy
  z=fz
  w=gw
}

DragonAlt1 { ; William McWorter
  Angle 8
  Axiom X
  F=
  X=FX+@.500FZ@2.00+FY
  Y=FX-@.500FZ@2.00-FY
  Z=FZ
}

DragonCurd { ; William McWorter
  Angle 4
  Axiom su
  f=
  g=
  s=fs+g-x
  t=++f--u+g-x
  u=++f--u-g+z
  v=fs-g+z
  w=gw+f-t
  x=++g--y+f-t
  y=++g--y-f+v
  z=gw-f+v
}

Dragonrnd { ; William McWorter
  Angle 8
  Axiom X
  F=
  X=FX+@.500FZ@2.00+FY
  Y=FX-@.500FZ@2.00-FY
  Z=FZ
}

Dragon&med { ; William McWorter
  Angle 8
  Axiom -c32X+[+++@.5gw---c40@q2fz]+c48y
  F=
  g=
  X=fx+[+++@.5gw---c40@q2fz]+fy
  Y=Fx-[---@.5gw+++c40@q2fz]-fy
  z=fz
  w=gw
}

Fass1 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
        ; FASS curve (3x3 tiles form macrotile), Figure 1.16a p.17
  axiom -l
  angle 4
  L=LF+RFR+FL-F-LFLFL-FRFR+
  R=-LFLF+RFRFR+F+RF-LFL-FR
}

Fass2 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
        ; FASS curve (4x4 tiles form macrotile), Figure 1.16b p.17
  angle 4
  axiom -l
  L=LFLF+RFR+FLFL-FRF-LFL-FR+F+RF-LFL-FRFRFR+
  R=-LFLFLF+RFR+FL-F-LF+RFR+FLF+RFRF-LFL-FRFR
}

FracRhombusTile { ; William McWorter
  Angle 12
  Axiom f
  f=[-f++g-]+f--g+
  g=[-f++g-]+f--g+
}

Frame { ; William McWorter
  ; made from Moore's spacefilling curve
  Angle 4
  Axiom YXY-YXY-YXY-YXY
  F=
  X=FX+FX+FXFYFX+FXFY-FY-FY-
  Y=+FX+FX+FXFY-FYFXFY-FY-FY
}

Grid { 
  Angle 4       ; Allow turns of 90=360/4 degrees
  Axiom f       ; Birth string is the symbol f (see text below)
  f=f[+f][-f]f  ; Replacement string for f (see text below)
}

Grid1 { ; William McWorter
  Angle 4
  Axiom x
  x=y
  y=f
  f=f[+f][-f]f
}

 GridMedian { ; William McWorter
  Angle 4
  Axiom z
  z=x
  x=-[c40@I2g+f+@2f+f+f+@I2f]yfw+[c40@I2g+f+@2f+f+f+@I2f]xfw
  x=[c40@I2g+f+@2f+f+f+@I2f]x+fw[c40@I2g+f+@2f+f+f+@I2f]y-
  y=+[c40@I2g+f+@2f+f+f+@I2f]xfw-[c40@I2g+f+@2f+f+f+@I2f]yfw
  y=[c40@I2g+f+@2f+f+f+@I2f]y-fw[c40@I2g+f+@2f+f+f+@I2f]x+
  w=fw
  f=
  g=
}

Hex { 
  Angle 6         ; Turns are 60=360/6 degrees
  Axiom f         ; Birth string is f
  f=-f+f+g[+f+f]- ; Replacement string for f 
  g=gg            ; Replacement string for g (see text below)
}

Hex1 { 
  Angle 6
  Axiom f
  f=-f+f+f[+f+f+f]-
}

Hilbert {       ; Ken Philip, from The Science of Fractal Images
  axiom x
  x=-YF+XFX+FY- ; cup interior on left during trace
  y=+XF-YFY-FX+ ; cup interior on right during trace
  angle 4
}

 Hilbert1 { 
  axiom z
  z=-c32Yc15f+c32Xc15Fc32X+c15Fc32Y-
  x=-YF+XFX+FY-
  y=+XF-YFY-FX+
  angle 4
}

Hiway { 
  Angle 4
  Axiom fx
  x=fx+fy
  y=fx-fy
  f=
}

 Hiway1 {
  Angle 8
  Axiom u
  F=
  g=
  u=v
  v=c32fX+[+++@.5gw---c40@q2fz]+c48fy
  X=fx+[+++@.5gw---c40@q2fz]+fy
  Y=Fx-[---@.5gw+++c40@q2fz]-fy
  z=fz
  w=gw
} 

Hiwayint { ; William McWorter
  Angle 4
  Axiom s
  F=
  s=x+F-t
  t=++F--u+F-t
  u=z-F+v
  v=Fs-F+v
  w=Fsw
  x=xy
  y=++F--uy
  z=zw
}

Hiwaymed {
  Angle 8  ; Smallest turn is 45=360/8 degrees
  Axiom -X ; Turn whole thing 45 degrees so that all lines of X are horizontal or vertical
  X=X+F+Y  ; Do X, turn left, forward, turn left, do Y
  Y=X-F-Y  ; Do X backwards
}

Hiwaymed-x { ; William McWorter
  Angle 4
  Axiom s
  F=
  s=Fs+F-t
  t=+F-u+F-t
  u=++F--v++F--w
  v=++F--v-F+x
  w=-F+y++F--w
  x=-F+y-F+x
  y=FsFz
  z=+F-uFz
}

Isocepent { ; William McWorter
  Angle 10
  Axiom w
  w=++[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw--
  w=[--@1.618033989f++++f]fy++
  x=-[+@1.618033989f@I1.618033989----f]fz++[--@1.618033989f++++f]fy-
  y=--[+@1.618033989f@I1.6180339898----f]fz++[--@1.618033989f++++f]fy++
  y=[++@1.618033989f----f]fw--
  z=+[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw+
  f=
}

Isocepent-1 { ; William McWorter
  Angle 10
  Axiom s
  s=w
  w=++[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw--
  w=[--@1.618033989f++++f]fy++
  x=-[+@1.618033989f@I1.618033989----f]fz++[--@1.618033989f++++f]fy-
  y=--[+@1.618033989f@I1.6180339898----f]fz++[--@1.618033989f++++f]fy++
  y=[++@1.618033989f----f]fw--
  z=+[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw+
  f=
}

Isocepent-2 { ; William McWorter
  Angle 10
  Axiom s
  s=z
  w=++[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw--
  w=[--@1.618033989f++++f]fy++
  x=-[+@1.618033989f@I1.618033989----f]fz++[--@1.618033989f++++f]fy-
  y=--[+@1.618033989f@I1.6180339898----f]fz++[--@1.618033989f++++f]fy++
  y=[++@1.618033989f----f]fw--
  z=+[-@1.618033989f@I1.618033989++++f]fx--[++@1.618033989f----f]fw+
  f=
}

isocright { 
  Angle 8
  Axiom x
  x=+y-f-y+
  y=-x+f+x-
}

KochCurve { ; Adrian Mariano
  Angle 6
  Axiom F
  F=F+F--F+F
}

Lace { ; William McWorter
  ; A 30-60-90 triangle dissects into three
  ; congruent 30-60-90 triangles
  ; This script traverses the centers of the congruent triangles
  Angle 12
  Axiom w
  w=+++x--F--zFx+
  x=---w++F++yFw-
  y=+zFx--F--z+++
  z=-yFw++F++y---
}

Leafy {
  Angle 50
  Axiom +++++++++++++c6x
  x=f[@.5+++++++++x@.5f+++++++c48f--f|++++f------f]
  x=-f[@.4-----------!x@.5f+++++++c48f--f|++++f------f]
  x=@.6x
}

LawnInSpring {
  Angle 16
  Axiom c49x
  x=[+++++f-f-fz]gx++++gy
  y=[+++++f-f-fz]gx----gy
  z=w
  w=u
  u=[c44@.3[+++f]g++[+++f]g++[+++f]g++[+++f]g++[+++f]g++[+++f]g++[+++f]g++[+++f]g]z
  f=
  g=
}

Maze01 {
  Angle 3
  Axiom c40f+c48f+c56f
  f=f+ff-f
}

Maze&Fractal1 {
  Angle 3
  Axiom x
  x=fy+fyfy-fy
  y=fx-fxfx+fx
  f=
}

Moore { ; William McWorter
  Angle 4
  Axiom X
  F=
  X=FX+FX+FXFYFX+FXFY-FY-FY-
  Y=+FX+FX+FXFY-FYFXFY-FY-FY
}

Peano1 { ; Adrian Mariano
  ; from The Fractal Geometry of Nature by Mandelbrot
  Angle 4
  Axiom F
  F=F-F+F+F+F-F-F-F+F
}

Peanomed { ; William McWorter
  Angle 8
  Axiom -X
  X=X+F+X-F-X-F-X-F-X+F+X+F+X+F+X-F-X
}

Pentant { ; William McWorter
  Angle 5
  Axiom x-x-x-x-x
  f=
  x=fx-fx-fx+fy+fy+fx-fx
  y=fy+fy-fx-fx-fy+fy+fy
}

PentaPlexity-0 {
  ; Manual construction by Roger Penrose as a prelude to his development of
  ; the famous Penrose tiles (the kites and darts) that tile the plane
  ; only non-periodically.
  ; Translated first to a "dragon curve" and finally to an L-system
  ; by Joe Saverino.
  Angle 10
  Axiom F++F++F++F++F
  F=F++F++F|F-F++F
}

Pentigree { ; William McWorter
  Angle 5
  Axiom F-F-F-F-F
  F=F-F++F+F-F-F
}

Pentive { 
  Angle 10
  Axiom W
  F=
  W=++FX--FW--FY++
  X=-FZ++FY-
  Y=--FZ++FY++FW--
  Z=+FX--FW+
}

Pentive1 {
  Angle 10
  Axiom x
  f=
  g=
  x=++fz----fy++fx
  y=+fw--gz+
  z=--fx++++fw--fz
  w=-fy++gx-
}

Pentive2 {
  Angle 10
  Axiom x
  f=
  x=fy++fz----fw++
  y=+fw--fu+
  z=++fv----fx++fy
  w=fu--fv++++fx--
  u=-fx++fy-
  v=--fz++++fw--fu
}

Pentive3 { 
  Angle 10
  Axiom q
  f=
  p=--fr++++fs--fu
  q=ft++fr----fs++
  r=++fp----fq++ft
  s=fu--fp++++fq--
  t=+fu--fp+
  u=-fq++ft-
}

Penty { ; William McWorter *
  Angle 10
  Axiom W
  F=
  W=++c15FX--c48FW--c48FY++
  X=-c15FZ++c40FY-
  Y=--c15FZ++c40FY++c48FW--
  Z=+c15FX--c40FW+
}

Pentl { ; William McWorter
  Angle 5
  Axiom f-f-f-f-f
  f=f-f-f++f+f-f
}

Plant04 { ; Adrian Mariano, from the Algorithmic Beauty of Plants
         ; Plant-like structure, figure 1.24d p.25
  angle 18
  axiom x
  X=F[+X]F[-X]+X
  F=FF
}

PreLace {
  Angle 12
  Axiom +++c32fw----c15fzc48fw+
  f=
  y=+++fw----fzfw+
  x=+fzfw----fz+++
  w=---fy++++fxfy-
  z=-fxfy++++fx---
}

RhombusTile {
  Angle 12
  Axiom f
  f=[-f++f-]+f--f+
}

Sierpinsk { ; William McWorter
  ; Isoceles right triangle dissects into two congruent
  ; isoceles right triangles
  Angle 8
  Axiom L
  L=+R-F-R+
  R=-L+F+L-
}

Sierpinski { ; William McWorter
  Angle 8
  Axiom L--F--L--F
  L=+R-F-R+
  R=-L+F+L-
}

Sierpinski1 { 
  Angle 8
  Axiom z
  z=x
  x=+[c40-@.5g++@2.414g|@q2f--f---@q2f]y-fw-
  x=[c40-@.5g++@2.414g|@q2f--f---@q2f]y+
  y=-[c40+@.5g--@2.414g|@q2f++f+++@q2f]x+fw+
  y=[c40+@.5g--@2.414g|@q2f++f+++@q2f]x-
  w=fw
  f=
  g=
}

SierpinskiCarpet {
  Angle 4
  Axiom f
  f=f+f-f-f-g+f+f+f-f
  g=ggg
}

Snowflake  {
  Angle 6       ; All turns are multiples of 60=360/6 degrees
  Axiom f--f--f ; Draw clockwise a ring of three Snowflakes
  f=f+f--f+f    ; Forward, turn left, forward, turn left, turn left, ... , forward
}

Strike_S { ; William McWorter
  ; Strikethrough S
  Angle 4
  Axiom UW
  F=
  U=FU+F-V
  V=++F--W++F--Z
  W=++F--W-F+X
  X=FUFY
  Y=+F-VFY
  Z=-F+X--F++Z
}

Terdragon { ; William McWorter
  ; from Davis and Knuth. Journal of
  ; Rec. Math. vol. 3, 1970, pages 61-81
  Angle 3
  Axiom F
  F=F+F-F
}

Terdragon-c { ; William McWorter
  ; from Davis and Knuth. Journal of
  ; Rec. Math. vol. 3, 1970, pages 61-81
  Angle 3
  Axiom c15F+c48F-c40F
  F=F+F-F
}

Terdragon1 {
  Angle 3
  Axiom x
  x=y
  y=c15F+c48F-c40F
  F=F+F-F
}

TerdragnM { ; William McWorter
  ; median terdragon
  Angle 6
  Axiom X
  X=X+F+X-F-X
}

ThreeTile {
  Angle 12
  Axiom [c32u]@q3"
  "=u
  u=[+++@iq3f----@2f]fx
  x=+++[---@iq3f++++@2f]fy----[-@2@iq3f++++@i2f]fz[---@iq3f++++@2f]fy+
  y=---[+++@iq3f----@2f]fx++++[+@2@iq3f----@i2f]fw[+++@iq3f----@2f]fx-
  z=-[+@2@iq3f----@i2f]fw[+++@iq3f----@2f]fx++++[+@2@iq3f----@i2f]fw---
  w=+[-@2@iq3f++++@i2f]fz[---@iq3f++++@2f]fy----[-@2@iq3f++++@i2f]fz+++
  f=
}

ThreeTile&med {
  Angle 12
  Axiom u
  u=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x
  x=+++[c32----g-----@1.366f---@q3f-----@2@iq3f]y--fp--
  x=[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]zfp
  x=[c32----g-----@1.366f---@q3f-----@2@iq3f]y+
  y=---[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x++fp++
  y=[c32++g-----@1.366f---@q3f-----@2@iq3f]wfp
  y=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x-
  z=-[c32++g-----@1.366f---@q3f-----@2@iq3f]wfp
  z=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x++fp++
  z=[c32++g-----@1.366f---@q3f-----@2@iq3f]w---
  w=+[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]zfp
  w=[c32----g-----@1.366f---@q3f-----@2@iq3f]y--fp--
  w=[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]z+++
  p=fp
  f=
  g=
}

ThreeTile&med1 {
  Angle 12
  Axiom u---fp---v---fp
  v=[c32++g-----@1.366f---@q3f-----@2@iq3f]w
  u=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x
  x=+++[c32----g-----@1.366f---@q3f-----@2@iq3f]y--fp--
  x=[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]zfp
  x=[c32----g-----@1.366f---@q3f-----@2@iq3f]y+
  y=---[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x++fp++
  y=[c32++g-----@1.366f---@q3f-----@2@iq3f]wfp
  y=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x-
  z=-[c32++g-----@1.366f---@q3f-----@2@iq3f]wfp
  z=[c32++++g+++++@1.366f+++@q3f+++++@2@iq3f]x++fp++
  z=[c32++g-----@1.366f---@q3f-----@2@iq3f]w---
  w=+[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]zfp
  w=[c32----g-----@1.366f---@q3f-----@2@iq3f]y--fp--
  w=[c32--g+++++@1.366f+++@q3f+++++@2@iq3f]z+++
  p=fp
  f=
  g=
}

Tiling1 {
  Angle 6
  Axiom x
  x=f-f-f+f+fx++f-f-f+f+fx--f-f-f+f+fx
  f=
}

Trigrid { 
  Angle 3
  Axiom f
  f=f+f-f-ff+f+f-f
}

Trigrid1 {
  Angle 6
  Axiom x
  x=fy[+fy][--fy]fy
  y=fx[++fx][-fx]fx
  f=
}

Tripeanomed { ; William McWorter
  Angle 6
  Axiom x
  x=x+f+x-f-x-f-x+f+x+f+x-f-x
}

TriPeano { ; William McWorter
  Angle 3
  Axiom f
  f=f+f-f-f+f+f-f
}

TwinDragonCurd1 {
  Angle 4
  Axiom fx+c32fx
  x=fx+gy
  y=fx-gy
  f=
  g=
}

TwinDragonCurd {
  Angle 4
  Axiom fx+fx
  x=fx+gy
  y=gx-fy
  f=
  g=
}

TwoTile {
  Angle 8
  Axiom [c32z]@q2a
  a=z
  z=[+@q2@i2f--f]fx
  x=+[-@q2@i2f++f]fy--[-@q2@i2f++f]fy+
  y=-[+@q2@i2f--f]fx++[+@q2@i2f--f]fx-
  f=
}

Untitled { ; William McWorter
  Angle 4
  Axiom x
  f=
  x=fx-fy-fx+fy+fx+fy-fx-fy+
  y=-fx+fy+fx-fy-fx-fy+fx+fy
}

Untitledmed { ; William McWorter
  Angle 8
  Axiom +x
  x=x-f-y-f-x+f+y+f+x+f+y-f-x-f-y
  y=x+f+y+f+x-f-y-f-x-f-y+f+x+f+y
}

Weed-0 {
  Angle 50
  Axiom +++++++++++++x
  x=f[@.5+++++++++x]-f[@.4-----------!x]x
}

Weed {
  Angle 50
  Axiom +++++++++++++x
  x=f[@.5+++++++++x]-f[@.4-----------!x]@.6x
}

Xmastree { ; William McWorter
  Angle 10
  Axiom p
  f=
  p=--<1fr++++<1fs--<1fu
  q=<1ft++<1fr----<1fs++
  r=++<1fp----<1fq++<1ft
  s=<1fu--<1fp++++<1fq--
  t=+<1fu--<1fp+
  u=-<1fq++<1ft-
}

Xmastree-0 { ; William McWorter
  Angle 10
  Axiom W
  F=
  W=++c15FX--c48FW--c48FY++
  X=-c15FZ++c40FY-
  Y=--c15FZ++c40FY++c48FW--
  Z=+c15FX--c40FW+
}
