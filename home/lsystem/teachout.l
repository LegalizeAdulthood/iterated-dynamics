; Date: Fri, 18 Apr 1997 11:19:47 -0500
; From: "William A McWorter Jr." <mcworter@midohio.net>
; Subject: Gary Teachout's lsystems
;
; Some of Gary's lsystems are so weird I added some of his comments in
; hopes somebody might understand what he's talking about.  His plant-like
; lsystems grow very very slowly but look more realistic than others I
; have seen.
;
; ======
;
; Gary Teachout's lsystems converted to Fractint lsystem syntax by William McWorter
; Those lsystems with a * by Gary's name were written by Gary.
;
; The dragon, c-curve, tri2m, leaf and fern, all use a delay to simulate size
; limit recursion.  Hex-7-b is just the Gosper tiling (FlowSnake) with
; gingerbread.  Peano-c is just the third form of the Peano curve, I included
; this one because I didn't see it among the other l-system files.
;
; The dragon, c-curve, and fern look best at high res and high orders.
; I like the leaf and maple best at the orders just lower than where they
; fill in solid.  Most of these require floating point mode.

dragon-sl-66 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\66@0.406736da@i0.406736/90@0.913545dy@i0.913545\24
   y=/24@0.913545dx@i0.913545\90@0.406736dp@i0.406736/66
   p=dq
   q=dr
   r=ds
   s=dt
   t=du
   u=dv
   v=dw
   w=dy
   a=db
   b=de
   e=dh
   h=dj
   j=dk
   k=dl
   l=dn
   n=dx
}


dragon-sl-58 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\58@0.529919da@i0.529919/90@0.848048dy@i0.848048\32
   y=/32@0.848048dx@i0.848048\90@0.529919dp@i0.529919/58
   p=dq
   q=dr
   r=dy
   a=db
   b=de
   e=dx
}


dragon-sl-52 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\52@0.615661da@i0.615661/90@0.788010dy@i0.788010\38
   y=/38@0.788010dx@i0.788010\90@0.615661dp@i0.615661/52
   p=dy
   a=dx
}


dragon-sl-38 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\38@0.788010dx@i0.788010/90@0.615661dp@i0.615661\52
   y=/52@0.615661da@i0.615661\90@0.788010dy@i0.788010/38
   p=dy
   a=dx
}


dragon-sl-32 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\32@0.848048dx@i0.848048/90@0.529919dp@i0.529919\58
   y=/58@0.529919da@i0.529919\90@0.848048dy@i0.848048/32
   p=dq
   q=dr
   r=dy
   a=db
   b=de
   e=dx
}


dragon-sl-25 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\25@0.906308dx@i0.906308/90@0.422618dp@i0.422618\65
   y=/65@0.422618da@i0.422618\90@0.906308dy@i0.906308/25
   p=dq
   q=dr
   r=ds
   s=dt
   t=du
   u=dv
   v=dw
   w=dy
   a=db
   b=de
   e=dh
   h=dj
   j=dk
   k=dl
   l=dn
   n=dx
}


Peano-c { ; Gary Teachout *
   Angle 8
   Axiom +z
   z=fx
   f=
   x=fx-fy-fx+fy+fx+fy+fx+fy+fx-fy-fx-fy-fx-fy-fx+fy+fx
   y=fy
}


Hex-7-b { ; Gary Teachout *
   Angle 12
   Axiom fx
   f=
   x=-f++f-x-f--f+y---f--f+y+f++f-x+++f++f-x-f++f-x+++f--f+y--
   y=+f++f-x-f--f+y+f--f+y---f--f+y---f++f-x+++f++f-x+++f--f+y
}


Maple1 { ; Gary Teachout *
   Angle 36
   Axiom +++++++++x
   x=@0.6x@i0.6----f-@0.5x@i0.5+++++++++f----@0.6x@i0.6++++++++++++++f
   x=---------@0.5x@i0.5+++++++++++++++++f++++++++++++++
}


Maple2 { ; Gary Teachout *
   Angle 36
   Axiom +++++++++x
   x=@0.6x@i0.6--f---@0.5x@i0.5+++++++f--@0.6x@i0.6++++++++++++++++f
   x=-----------@0.5x@i0.5+++++++++++++++f++++++++++++++++
}


Maple3 { ; Gary Teachout *
   Angle 36
   Axiom +++++++++x
   x=+@0.6x@i0.6-----f-@0.5x@i0.5+++++++++f-----@0.58!x!@i0.58++++++
   x=+++++++++f----------@0.51!x!@i0.51++++++++++++++++++f++++++++++++++
}

Maple4 { ; Gary Teachout *
   Angle 36
   Axiom +++++++++x
   x=+@0.6!x!@i0.6-----f-@0.45x@i0.45+++++++++f-----@0.55!x!@i0.55++++
   x=+++++++++++f----------@0.51!x!@i0.51++++++++++++++++++f++++++++++++++
}


Fern1 { ; Gary Teachout *
   Angle 48
   Axiom ++++++++++++++fx
   x=f+++++++++y--------f@0.8x@i0.8------------------------f++++++++++
   x=+++++++!y!------------------f++++++++++++++++++++++++
   y=z
   z=p
   p=q
   q=@0.35x@i0.35
}

Fern2 { ; Gary Teachout *
   Angle 48
   Axiom +++++++++++++++fx
   x=f+++++++++y--------f@0.8x@i0.8-------!y!-----------------
   x=f-f++++++++++++++++++++++++
   y=z
   z=p
   p=q
   q=@0.35x@i0.35
}

Fern3 { ; Gary Teachout *
   Angle 48
   Axiom +++++++++++++++fx
   x=f+++++++++y--------f@0.8+x-@i0.8-------!y!-----------------
   x=f-f++++++++++++++++++++++++
   y=z
   z=p
   p=q
   q=@0.35x@i0.35
}

Tri2m-1 { ; Gary Teachout *
   Angle 8
   Axiom x
   d=
   x=\38!dx!/38dz/52!dp!\52
   p=dx
   z=dz
}

Tri2m-2 { ; Gary Teachout *
   Angle 8
   Axiom x
   d=
   x=\35!dx!/35dz/55!dp!\55
   p=dq
   q=dx
   z=dz
}

Tri2m-2-b { ; Gary Teachout *
   Angle 8
   Axiom x
   d=
   x=dz\35!dx!/35dz/55!dp!\55
   p=dq
   q=dx
   z=dz
}


Tri2m-2-c { ; Gary Teachout *
   Angle 8
   Axiom x
   x=\35!dx!/90!dp!\55
   p=dq
   q=dx
}

Tri2m-3 { ; Gary Teachout *
   Angle 8
   Axiom x
   d=
   x=\32!dx!/32dz/58!dp!\58
   p=dq
   q=dr
   r=dx
   z=dz
}

c-curve-sl-38 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\38@0.788010dx@i0.788010/90@0.615661da@i0.615661\52
   a=dx
}

c-curve-sl-32 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\32@0.848048dx@i0.848048/90@0.529919da@i0.529919\58
   a=db
   b=de
   e=dx
}

c-curve-sl-25 { ; Gary Teachout *
   Angle 12
   Axiom x
   d=
   x=\25@0.906308dx@i0.906308/90@0.422618da@i0.422618\65
   a=db
   b=de
   e=dh
   h=dj
   j=dk
   k=dl
   l=dn
   n=dx
}

leaf1 { ; Gary Teachout *
   Angle 4
   Axiom \90x
   x=@0.01d@i0.01\35p/35@0.04d@i0.04@0.85!x!@i0.85\180@0.05d@i0.05/180
   p=q
   q=y
   y=@0.55x@i0.55
}

leaf2 { ; Gary Teachout *
   Angle 4
   Axiom \90x
   x=@0.01d@i0.01\35p/35/40r\40@0.04d@i0.04@0.85!x!@i0.85\180@0.05d@i0.05/180
   p=q
   q=y
   y=@0.55x@i0.55
   r=s
   s=t
   t=u
   u=v
   v=w
   w=a
   a=b
   b=z
   z=@0.25x@i0.25
}

IsoTrap4a { ; Gary Teachout *
   Angle 12
   Axiom x
   x=p
   p=++@1.732f@i1.732!p!-fz--fr---@1.732fz@i1.732++++@1.732f@i1.732!q!@1.732fz@i1.732++++@1.732f@i1.732!q!----
   q=++++@1.732f@i1.732!p!----@1.732fz@i1.732@1.732f@i1.732!p!----@1.732fz@i1.732+++fs++fz+@1.732f@i1.732!q!--
   r=+++++@1.732f@i1.732!p!----@1.732fz@i1.732@1.732f@i1.732!p!-fz--f!s!--fz-@1.732f@i1.732q+++++
   s=-----@1.732f@i1.732p+fz++f!r!++fz+@1.732f@i1.732!q!@1.732fz@i1.732++++@1.732f@i1.732!q!-----
   z=fz
   f=
}

IsoTrap4b { ; Gary Teachout *
   Angle 12
   Axiom +++++r
   p=++@1.732f@i1.732!p!-fz--fr---@1.732fz@i1.732++++@1.732f@i1.732!q!@1.732fz@i1.732++++@1.732f@i1.732!q!----
   q=++++@1.732f@i1.732!p!----@1.732fz@i1.732@1.732f@i1.732!p!----@1.732fz@i1.732+++fs++fz+@1.732f@i1.732!q!--
   r=+++++@1.732f@i1.732!p!----@1.732fz@i1.732@1.732f@i1.732!p!-fz--f!s!--fz-@1.732f@i1.732q+++++
   s=-----@1.732f@i1.732p+fz++f!r!++fz+@1.732f@i1.732!q!@1.732fz@i1.732++++@1.732f@i1.732!q!-----
   z=fz
   f=
}

c-curve-1 { ; Gary Teachout *
   Angle 8
   Axiom f
   f=-f++f-
}

c-curve-1.5 { ; Gary Teachout *
   Angle 8
   Axiom f
   f=-@1.5f@i1.5++f-
}

dragon-1.2 { ; Gary Teachout * ; 39.8 degrees
   Angle 8
   Axiom x
   f=
   x=@1.2fx@i1.2--fy
   y=fx++@1.2fy@i1.2
}

dragon-0.75 { ; Gary Teachout * ; 53.1 degrees
   Angle 8
   Axiom x
   f=
   x=@0.75fx@i0.75--fy
   y=fx++@0.75fy@i0.75
}

Tri-2-d { ; Gary Teachout *
   Angle 8
   Axiom r
   r=x
   x=+uf--y-@0.707f@i0.707++
   v=--@0.707f@i0.707+u++fy-
   u=-xf++v+@0.707f@i0.707--
   y=++@0.707f@i0.707-x--fv+
}

Tri-2-e { ; Gary Teachout *
   Angle 8
   Axiom r
   r=c40x
   x=+fufz--fy-@0.707fz@i0.707++
   v=--@0.707fz@i0.707+fu++fzfy-
   u=-fxfz++fv+@0.707fz@i0.707--
   y=++@0.707fz@i0.707-fx--fzfv+
   f=
   z=fz
}


Tri-99-a  { ; Gary Teachout 
   Angle 24
   Axiom x
   x=++++y----f----z|---@q2f@iq2+++z--f------x---@q2f@iq2-----y+++@q2f@iq2+++++x++++f++++w|+++@q2f@iq2---w++f++x
   y=----x++++f++++w|+++@q2f@iq2---w++f++++++y+++@q2f@iq2+++++x---@q2f@iq2-----y----f----z|---@q2f@iq2+++z--f--y
   z=z--f--y+++@q2f@iq2|---y----f----z-----@q2f@iq2---w+++++@q2f@iq2+++z++++++f++x---@q2f@iq2|+++x++++f++++w----
   w=w++f++x---@q2f@iq2|+++x++++f++++w+++++@q2f@iq2+++z-----@q2f@iq2---w------f--y+++@q2f@iq2|---y----f----z++++
}

Tri-9-4-f { ; Gary Teachout
   Angle 12
   Axiom x
   x=fa+fp+++fb-fp-fb-fp++fp---fa---fp-fb+++fp+fa---fp---fa+fp+++fb+++fp--fp+fa
   y=fb-fp---fa+fp+fa+fp--fp+++fb+++fp+fa---fp-fb+++fp+++fb-fp---fa---fp++fp-fb
   a=++fy-fp++fp---fx---fp-fy+++fp+fx
   b=--fx+fp--fp+++fy+++fp+fx---fp-fy
   p=fp
   f=
}

Tri-4-9-f { ; Gary Teachout
   Angle 12
   Axiom r
   r=x
   x=++fb-fp++fp---fa---fp-fb+++fp+fa ; ++fy-fp++fp---fx---fp-fy+++fp+fx
   y=--fa+fp--fp+++fb+++fp+fa---fp-fb ; --fx+fp--fp+++fy+++fp+fx---fp-fy
   a=fx+fp+++fy-fp-fy-fp++fp---fx---fp-fy+++fp+fx---fp---fx+fp+++fy+++fp--fp+fx
   b=fy-fp---fx+fp+fx+fp--fp+++fy+++fp+fx---fp-fy+++fp+++fy-fp---fx---fp++fp-fy
   p=fp
   f=
}

Tri-9-4-a  { ; Gary Teachout
   Angle 6
   Axiom r
   r=x
   x=
}

Tri-4-9-a { ; Gary Teachout
   Angle 6
   Axiom r
   r=z
   x=fpa+fp+fz-fp-fz-fp-fw+fp+fz
   y=fpa-fp-fw+fp+fw+fp+fz-fp-fw
   z=ffffppppbb+fp+fpb+fp+fx-fp-fx-fp-fy+fp+fx-fp-fx-fp-fy+fp+fx-fp-fy+fp+fx
   w=ffffppppbb-fp-fpb-fp-fy+fp+fy+fp+fx-fp-fy+fp+fy+fp+fx-fp-fy+fp+fx-fp-fy
   a=ffffffppppppbbb
   b=fffpppaa
   p=fp
   f=
}


Tri-9-f { ; Gary Teachout
   Angle 12
   Axiom x
   x=fx+fp+++fy-fp-fy-fp++fp---fx---fp-fy+++fp+fx---fp---fx+fp+++fy+++fp--fp+fx
   y=fy-fp---fx+fp+fx+fp--fp+++fy+++fp+fx---fp-fy+++fp+++fy-fp---fx---fp++fp-fy
   p=fp
   f=
}

Tri-9-b { ; Gary Teachout
   Angle 6
   Axiom x
   x=fp+fp-fx-fp+fp+fy+fp+fx-fp-fy+fp-fp-fx-fp-fy+fp+fx-fp-fy+fp+fx
   y=fp-fp+fy+fp-fp-fx-fp-fy+fp+fx-fp+fp+fy+fp+fx-fp-fy+fp+fx-fp-fy
   p=fp
   f=
}

Tri-9-a { ; Gary Teachout
   Angle 6
   Axiom x
   x=ffffwwpp+fp+fw+fp+fx-fp-fx-fp-fy+fp+fx-fp-fx-fp-fy+fp+fx-fp-fy+fp+fx
   y=ffffwwpp-fp-fw-fp-fy+fp+fy+fp+fx-fp-fy+fp+fy+fp+fx-fp-fy+fp+fx-fp-fy
   w=fffffffppppwww
   p=fp
   f=
}

Rect-5 { ; Gary Teachout
   Angle 4
   Axiom x
   x=+fy-fp-fx+fp+fy-fp-fx+fp+fy-
   y=-fx+fp+fy-fp-fx+fp+fy-fp-fx+
   p=fp
   f=
}

Rect-2 { ; Gary Teachout
   Angle 4
   Axiom x
   x=+fy-fp-fz+fp+fy-
   y=-fx+fp+fz-fp-fx+
   z=ffppww
   w=fz
   p=fp
   f=
}

Rect-3 { ; Gary Teachout
   Angle 4
   Axiom x
   x=+fy-fp-fx+fp+fy-
   y=-fx+fp+fy-fp-fx+
   f=
   p=fp
}

Tri-4-a { ; Gary Teachout
   Angle 6
   Axiom fx                ; x
   x=z+f+fx-f-fx-f-fy+f+fx ; fz+fp+fx-fp-fx-fp-fy+fp+fx
   y=z-f-fy+f+fy+f+fx-f-fy ; fy-fp-fx+fp+fy+fp+fy-fp-fz
   z=fffzz                 ; z=ffffzzpp
   ; f=
   ; p=fp
}

Tri-4-b { ; Gary Teachout
   Angle 6
   Axiom x
   x=fp+fy+fp-fp-fx-fp-fy+fp+fx
   y=fy-fp-fx+fp+fy+fp+fp-fx-fp
   p=fp
   f=
}

Cross5c { ; Gary Teachout.  Same as Quartet in fractint.l
   Angle 4
   Axiom x
   x=fx+fy-fx-fxfy+
   y=-fxfy+fy+fx-fy
   f=
}

Tri-4-d { ; Gary Teachout
   Angle 12
   Axiom x
   x=xfy++f++x---f---y-@1.366f@i1.366+++
   y=+++@1.366f@i1.366-x---f---y++f++xfy
}

Tri-4-e { ; Gary Teachout
   Angle 12
   Axiom x
   x=x+f+++y-f---x--@q3f@iq3y++
   y=y-f---x+f+++y++@q3f@iq3x-- ; --x@q3f@iq3++y+++f+x---f-y
}

Tri-4-f { ; Gary Teachout
   Angle 12
   Axiom x
   x=++fy-fp++fp---fx---fp-fy+++fp+fx
   y=--fx+fp--fp+++fy+++fp+fx---fp-fy ; fy--fp--fx+fp+++fy+++@1.25fp--fp@i1.25+fx--
   f=
   p=fp
}
