Dragon0001 {
   Angle 4
   Axiom fx
   x=x+yf+fx-yf+fx+yf-fx-yf-fx+yf+fx-yf-fx+yf-fx-yf
   y=fx+yf+fx-yf+fx+yf-fx-yf+fx+yf+fx-yf-fx+yf-fx-y
}

Dragon010 {
   Angle 4
   Axiom fx
   x=x+yf-fx-yf+fx+yf+fx-yf
   y=fx+yf-fx-yf-fx+yf+fx-y
}

Dragon001med {
   Angle 8
   Axiom +x
   x=x+f+y+f+x-f-y-f-x+f+y-f-x-f-y
   y=x+f+y+f+x-f-y+f+x+f+y-f-x-f-y
}

Dragon001 {
   Angle 4
   Axiom r
   r=c40fx-<8yf-<8fx-<4yf
   x=x+yf+fx-yf-fx+yf-fx-yf
   y=fx+yf+fx-yf+fx+yf-fx-y
}

3Tile&med {
   Angle 12
   Axiom r
   r=x
   p=fp
   f=
}

Sextet {
   Angle 10
   Axiom r
   r=x
   x=fx++fy--fx--fx--fx+fy+++
   y=---fx-fy++fy++fy++fx--fy
   f=
   p=fp
}

TrigridMed {
   Angle 12
   Axiom r
   r=x
   x=++z+f--f-x---f-y+++f+x
   y=--w-f++f+y+++f+x---f-y
   z=z-f---w+f+++z+f++f-x--
   w=w+f+++z-f---w-f--f+y++
}

Pentive&Med {
   Angle 10
   Axiom w
   w=++fxfp--fw--fpfy++
   x=-fz+@.618fp@i.618+fy-
   y=--fzfp++fy++fpfw--
   z=+fx-@.618fp@i.618-fw+
   f=
   p=fp
}

Trigrid&Med { 
   Angle 6
   Axiom r
   r=xf
   x=+fyff-xf-f-fy+f+x
   y=y-f-xf+f+fy+ffxf- ; -fxfpfp+fy+fp+fx-fp-fy
}

PreLace&Med { 
   Angle 12
   Axiom r
   r=s
   s=+++c40fw--c15fp--c48fzc15fpc56fw+
   y=+++fw--fp--fzfpfw+
   x=+fzfpfw--fp--fz+++
   w=---fy++fp++fxfpfy-
   z=-fxfpfy++fp++fx---
   p=fp
   f=
}

IBeam {
   Angle 4
   Axiom r
   r=c40x+<8y+<8x+<4y
   x=+fy-fxfyfx-fy+
   y=-fx+fyfxfy+fx-
   z=+fz-fwfxfw-fz+
   w=-fz+fwfxfw+fz-
   f=
}

2Tile1Med {
   Angle 8
   Axiom +r
   r=x
   x=z-f-y
   y=x-f-w
   z=x+f+w
   w=z+f+y
}

2Tile1 {
   Angle 4
   Axiom r
   r=fx
   x=z-yf
   y=fx-w
   z=x+wf
   w=fz+y
}

Dragonmed1 {
   Angle 4
   Axiom r
   r=fx
   x=x+fyf
   y=fxf-y
}

TerdragonAlt {
   Angle 12
   Axiom r
   r=x
   x=-g+[+++f]+g-x+++g+[+++f]+g-x-----g+[+++f]+g-x
   f=
   g=
}

Dragon2 {
   Angle 4
   Axiom r
   r=fx
   x=x+yf
   y=fx-y
}

Dragon1 {
   Angle 4
   Axiom r
   r=fx
   x=x+yf+
   y=-fx-y
}

Pinetree {
   Angle 18
   Axiom vzfff
   v=[+++w][---w]yv
   w=+x[-w]z
   x=-w[+x]z
   y=yz
   z=[-fff][+fff]f
}

A {
   Angle 6
   Axiom r
   r=g-g--c40f+fc15f-ff+fc56f-fc15++g+gx
   x=g-g--c40f+fc15f-ff+fc56f-fc15++g+gx++g-g--c40f+fc15f-ff+fc56f-fc15++g+gx--g-g--c40f+fc15f-ff+fc56f-fc15++g+gx
   f=
   g=
}

A1 { ; Terdragon median based on a modification
   ; of the triangle tiling
   Angle 6
   Axiom r
   r=c40f-ff+fx+fp+fp<8f-ff+fx+fp+fp<8f-ff+fx+fp+fp
   x=f-ff+fx+fp+fpf-ff+fxfp-fp-f-ff+fx
   p=fp
   f=
   g=
}

TrapezoidTile1 {
   Angle 8
   Axiom r
   r=x
   x=x-@q2f@iq2+x-@q2f@iq2+ff++ff|x--yffff|
   y=y+@q2f@iq2-y+@q2f@iq2-ff--ff|y++xffff|
   f=gg
   g=gg
}

TrapezoidTile {
   Angle 6
   Axiom r
   r=x
   x=fxfff++xff+xff+xff++ ; x=+yff-yff-yff--fyfff|
   ;        y=-xff+xff+xff++fxfff|
   f=gg
   g=gg
}

TriominoTile {
   Angle 4
   Axiom r
   r=x
   x=xff[g+x]+f+f|x|-f+f[-g|x]+ff+
   f=g
   g=gg
}

Twindragonint {
   Angle 8
   Axiom +x++x
   x=x++y
   y=x-f-y
}

Hiwayint1 {
   Angle 8
   Axiom x
   x=x++y
   y=x-f-y
}

TreeMesh {
   Angle 6
   Axiom r
   r=x
   x=xzf[-wf+x]+wf+y--
   y=xzf[+wf+x]-wf+y
   z=[c40+g++f]
   w=[c40-g+f]
}

SpacefillingTree {
   Angle 6
   Axiom r
   r=x
   x=xf[-f+x]+f+y--
   y=xf[+f+x]-f+y
}

SquareGrid {
   Angle 4
   Axiom x
   x=[x]f+[x]f+[x]f+[x]f+
   f=g
   g=gg
}

Barbells {
   Angle 16
   Axiom x
   x=-f++f-x+++++f--f+y
   y=-f++f-x----+f--f+y
   f=
}

2Isosceles {
   Angle 10
   Axiom r
   r=x
   x=[@i1.618f---@1.618f]--fz+++[@i1.618f++f]+fw--[--@i1.618f+++@1.618f]fo
   y=[f+++@i1.618f]+fq--[-@i1.618f++f]fs+
   z=[@i1.618f+++@1.618f]++fx---[@i1.618f--f]-fy++[++@i1.618f---@1.618f]fn
   w=[f---@i1.618f]-ft++[+@i1.618f--f]fp-
   u=[-f+++@i1.618f]fq--[-@i1.618f++f]fs++++[+f---@i1.618f]fv--
   v=[+f---@i1.618f]ft++[+@i1.618f--f]fp----[-f+++@i1.618f]fu++
   t=+[-f+++@i1.618f]fq--[-@i1.618f++f]fs++++[+f---@i1.618f]fv|g++
   s=[@i1.618f--f]-fy++[++@i1.618f---@1.618f]fn-
   q=-[+f---@i1.618f]ft++[+@i1.618f--f]fp----[-f+++@i1.618f]fu|g--
   p=[@i1.618f++f]+fw--[--@i1.618f+++@1.618f]fo+
   o=--g|[--@i1.618f+++@1.618f]fx---[@i1.618f--f]-fy++[++@i1.618f---@1.618f]fn-
   n=++g|[++@i1.618f---@1.618f]fz+++[@i1.618f++f]+fw--[--@i1.618f+++@1.618f]fo+
   f=
   g=+g--g+
}

Pentive4 { ; from 2isosceles
   Angle 10
   Axiom r
   r=x
   x=--fz++++fw--fo
   y=+fq--fs+
   z=++fx----fy++fn
   w=-ft++fp-
   u=fq--fs++++fv--
   v=ft++fp----fu++
   t=+fq--fs++++fv|g++
   s=-fy++fn-
   q=-ft++fp----fu|g--
   p=+fw--fo+
   o=--g|fx----fy++fn-
   n=++g|fz++++fw--fo+
   f=
   g=+g--g+
}

Weave {
   Angle 4
   Axiom z
   z=x
   x=--g+g+@7f@i7+g+g++x+--g-g-@7f@i7-g-g++y
   y=--g+g+@7f@i7+g+g++x---g-g-@7f@i7-g-g++y
   f=
   g=
}

Starstruck {
   Angle 10
   Axiom v
   v=x
   x=[---fy][-fz]fy++fx----fy++fx
   y=fy++fx----fy++fx[--fz][----fy]
   z=fz--fw++++fz--fw[++fy][++++fz]
   w=[+++fz][+fy]fz--fw++++fz--fw
   f=
}

RhombusTile1 {
   Angle 12
   Axiom y
   x=-fx++fx[++++fy]-
   y=+fy--fy[----fx]+
   f=
}

TrainSmoke {
   Angle 12
   Axiom r
   r=f
   f=+f--f[----f]+
}

TheKissM {
   Angle 6
   Axiom x
   x=x+f+x-f-x-f-x+f+x-f-x+f+x+f+x-f-x
}

TheKiss {
   Angle 3
   Axiom f
   f=f+f-f-f+f-f+f+f-f
}


Schneider { ; Craig Schneider rendered for fractint
   Angle 12
   Axiom a
   a=f[-a][+a]fa
   f=ff
}

Alfonseca { ; Manuel Alfonseca rendered for fractint
   Angle 16
   Axiom A
   A=f-[A+A]+f[+fA]-A
   f=ff
}

Alfonseca1 { ;Manuel Alfonseca rendered for fractint
   Angle 12
   Axiom t
   f=ff
   l=[c48s+f-f-f+|+f-f-f]
   r=f[--l][++l]f
   t=r+[t]--[--l]r[++l]-[t]++t
   s=@.5s
}

Maze09rnd {
   Angle 6
   Axiom x
   x=fx+fp+fx-fp-fx-fp-fxfpfpfx+fp+fx+fp+fx-fp-fxfpfpfx
   p=fp
   f=
}

Maze09M {
   Angle 6
   Axiom x
   x=x+f+x-f-x-f-xffx+f+x+f+x-f-xffx
}

Maze09 {
   Angle 3
   Axiom f
   f=f+f-f-ff+f+f-ff
}

Maze08 {
   Angle 4
   Axiom x
   x=fx-fy+fx-fy+fx+fx-fy+fx+fxfy-fy-fx+fy-
   y=+fx-fy+fx+fxfy-fy-fx+fy-fy-fx+fy-fx+fy
   f=
}

Maze07 {
   Angle 4
   Axiom x
   x=fxfy+fyfxfy+fyfx-fx-fxfyfx-fxfy+ 
   y=-fxfy+fyfxfy+fy+fyfx-fxfyfx-fxfy 
   f=
}

Maze06M {
   Angle 8
   Axiom x
   x=x-f-y+f+x-f-y-f-x
   y=y+f+x+f+y-f-x+f+y
}

Maze06 {
   Angle 4
   Axiom x
   x=fx-fy+fx-fy-fx++
   y=--fy+fx+fy-fx+fy
   f=
}

Maze05M {
   Angle 8
   Axiom x
   x=x+f+y+f+x-f-y-f-x-f-y+f+x-f-y+f+x+f+y
   y=x-f-y-f-x+f+y-f-x+f+y+f+x+f+y-f-x-f-y
}

Maze05 {
   Angle 4
   Axiom x
   x=fx+fy+fx-fy-fx-fy+fx-fy+fx+fy-
   y=+fx-fy-fx+fy-fx+fy+fx+fy-fx-fy
   f=
}

Maze04M {
   Angle 6
   Axiom x
   x=x+f+xffx-f-x-f-x+f+x-f-x+f+x+f+x-f-x+f+x-f-x
}

Maze04 {
   Angle 3
   Axiom f
   f=f+ff-f-f+f-f+f+f-f+f-f
}

Maze03 {
   Angle 4
   Axiom x
   x=fxfy+fy+fyfx-fx-fxfy+fyfx-fx-fxfy+
   y=-fxfy+fy+fyfx-fxfy+fy+fyfx-fx-fxfy
   f=
}

Maze&Fractal1rnd {
   Angle 6
   Axiom x
   x=fy+fp+fyfpfpfy-fp-fy
   y=fx-fp-fxfpfpfx+fp+fx
   p=fp
   f=
}

Maze&Fractal1M {
   Angle 6
   Axiom x
   x=y+f+yffy-f-y
   y=x-f-xffx+f+x
}

Maze01rnd {
   Angle 6
   Axiom x
   x=fx+fp+fxfpfpfx-fp-fx
   p=fp
   f=
}

Maze01M {
   Angle 6
   Axiom y
   y=x
   x=x+f+xffx-f-x
}

Maze&Fractal2M {
   Angle 6
   Axiom y
   y=x
   x=x+f+xffx+f+x-f-xffx-f-x
}

Maze&Fractal2 {
   Angle 3
   Axiom f
   f=f+ff+f-ff-f
}

Maze&Fractal2rnd {
   Angle 6
   Axiom fx
   x=fx+fp+fxfpfpfx+fp+fx-fp-fxfpfpfx-fp-fx
   p=fp
   f=
}

Maze02 {
   Angle 4
   Axiom fx
   x=+fy+fyfx-fx-fxfyfx-fxfy+fy+fyfx-fx
   y=fy+fyfx-fx-fxfy+fyfxfy+fy+fyfx-fx-
   f=
}
