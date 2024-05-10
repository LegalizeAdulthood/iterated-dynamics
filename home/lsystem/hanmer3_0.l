;
; It's amazing how living in a place where the electricity exists for about 6 
; hours a day, and where one doesn't have instant computer access, can be 
; conducive to "thought experiments".
;
; The sets of  l-systems (adh119-123) which I am sending separately are all 
; derived from only one original formula, upon which I stumbled on Feb. 13, 
; and then thought about extensively when the power was off.  I varied the 
; angles from the original 4 (90 degrees) to 3, 5, 6 and 8, and played with 
; the formula each time until the results were satisfactory, and deleted the 
; failures.  Most of the pieces have n-fold or n/2-fold rotational symmetry (n 
; being the Angle), but a number of them also change this symmetry from order 
; to order.
;
; I certainly wouldn't say that I have mastered l-systems yet - I'm still 
; playing, and rarely can I actually produce a formula which does what I WANT. 
;
; The alternative - spewing out experiments and deleting the unsuccessful 
; ones - is enough for now.
;
; Enjoy - simpler than other formula types but capable of surprising richness 
; nonetheless.
;
; Tony Hanmer
; Tbilisi, Georgia
;
;
ADH105d2a { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f+f+f+f-f-f-f
}

ADH105d3 { ; Anthony Hanmer 2000
   Angle 6
   Axiom +f
   f=f+f+f+f+f-f-f-f-f
}

ADH105d3b { ; Anthony Hanmer 2000
   Angle 6
   Axiom +f
   f=g+f+g+f+g-f-g-f-g
}

ADH105d4b { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=+f+f+f+f-f-f-f-f
}

ADH105d5 { ; Anthony Hanmer 2000
   Angle 6 ; Cracks
   Axiom f
   f=+f+f-f+f-f+
}

ADH105d5a { ; Anthony Hanmer 2000
   Angle 3
   Axiom f
   f=+f+f-f+
}

ADH105d5c { ; Anthony Hanmer 2000
   Angle 6 ; Cracks
   Axiom f+f+f+f+f+f
   f=+f+f-f+f-f+
}

ADH105d5d { ; Anthony Hanmer 2000
   Angle 3
   Axiom f
   f=f+ff-gf
}

ADH105d5f { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+ff-ff
}

ADH105d5g { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-ff
}

ADH105d5h { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=+f+f-f+
}

ADH105m4 { ; median version of Anthony Hanmer's 105d, 2000
   Angle 8 ; playing
   Axiom -x
   x=x+f+x+g+x-f-x-g-x
}

ADH105r4 { ; rounded version of Anthony Hanmer's 105d, 2000
   Angle 8 ; playing
   Axiom x
   f=
   y=fy
   x=fx+@.5fy@2+gx+@.5gy@2+fx-@.5fy@2-gx-@.5gy@2-fx
}

ADH105r5 { ; rounded version of Anthony Hanmer's 105d, 2000
   Angle 8 ; playing
   Axiom x
   f=
   y=fy
   x=fx+@.5gy@2+fx+@.5gy@2+fx-@.5gy@2-fx-@.5gy@2-fx
}

ADH109 { ; Anthony Hanmer 2000
   Angle 4
   Axiom +f
   f=f+h-f+
   h=h-f+h-
}

ADH109b { ; Anthony Hanmer 2000
   Angle 6
   Axiom +f+f+f+f+f+f
   f=f+h-f+
   h=h-f+h-
}

ADH109c { ; Anthony Hanmer 2000
   Angle 4
   Axiom +f+f+f+f
   f=f+h-f+
   h=h-f+h-
}

ADH109d { ; Anthony Hanmer 2000
   Angle 3
   Axiom +f
   f=f+h-f+
   h=h-f+h-
}

ADH109f { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f-f+f+f-f+f+f-f
   f=f+h-f+
   h=h-f+h-
}

ADH109g { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f+f-f-f+f-f-f+f
   f=f+h-f+
   h=h-f+h-
}

ADH109h { ; Anthony Hanmer 2000
   Angle 3
   Axiom -f-f-f
   f=f+h-f+
   h=h-f+h-
}

ADH114 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f--h+f--h+f--h+f--h
   f=f+@.8f--g+@1.25f
   h=f-@.8f++g-@1.25f
}

ADH115 { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+@.8f--g+@1.25f
}

ADH115a { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+@.8f--g+@1.25f
}

ADH118col { ; Anthony Hanmer 2000
   Angle 3
   Axiom c09f-c10h-c09f-c10h-c09f-c10h
   f=f+f-f
   h=f-f+f
}

ADH118col2 { ; Anthony Hanmer 2000
   Angle 3
   Axiom c09f-c10h-c11f-c12h-c13f-c14h
   f=f+f-f
   h=f-f+f
}

ADH124a { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f-f+f+f-f
}

ADH124a1 { ; Anthony Hanmer 2000
   Angle 4 ; Odd orders cancel to a blank screen -
   Axiom f-f-f-f ; only even orders show...
   f=h-h+g+h-h
   h=f+f-g-f+f
}

ADH124a2 { ; Anthony Hanmer 2000
   Angle 6 ; Odd orders cancel to a blank screen -
   Axiom f-f-f-f-f-f ; only even orders show...
   f=h--h+g+h--h
   h=f++f-g-f++f
}

ADH124a3 { ; Anthony Hanmer 2000
   Angle 6 ; Odd orders cancel to a blank screen -
   Axiom f-f-f-f-f-f ; only even orders show...
   f=h-g++h++g-h
   h=f+g--f--g+f
}

ADH124a4 { ; Anthony Hanmer 2000
   Angle 6 ; Odd orders cancel to a blank screen -
   Axiom f-f-f-f-f-f ; only even orders show...
   f=h--h++h++h--h
   h=f++f--f--f++f
}

ADH124b { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f--f++f++f++f++f--f
}

ADH124c { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f-f+g++f+g+f-f
}

ADH124d { ; Anthony Hanmer 2000
   Angle 6 ; Modified Snowflake
   Axiom f+f+f+f+f+
   f=f--f+f--f
}

ADH124e { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-f+f-f
}


ADH125 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f
s f=ff--g+g++ff++g
}

ADH125a { ; Anthony Hanmer 2000
   Angle 4 ; Square of 125
   Axiom f-f-f-f
   f=ff--g+g++ff++g
}

ADH125b { ; Anthony Hanmer 2000 Spiral of 125
   Angle 4 ; (Add -@.5f to the axiom as many times as desired)
   Axiom f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f-@.5f
   f=ff--g+g++ff++g
}

ADH125c { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f--g+g++ff++g
}

ADH126 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f
   f=ff--g+g++f
}

ADH126a { ; Anthony Hanmer 2000
   Angle 4
   Axiom f
   f=ff-g+g+f
}

ADH126b { ; Anthony Hanmer 2000
   Angle 4
   Axiom f
   f=f+g++ff
}

ADH127b { ; Anthony Hanmer 2000
   Angle 4
   Axiom f--f--f--f
   f=+ff--g-g+ff
}

ADH127c { ; Anthony Hanmer 2000
   Angle 4
   Axiom 
f--@.75f--@.75f--@.75f--@.75f--@.75f--@.75f--@.75f--@.75f--@.75f--@.75f
   f=+ff--g-g+ff
}

ADH127d { ; Anthony Hanmer 2000
   Angle 3
   Axiom f
   f=+ff-g-g+ff
}

T129 { ; Anthony Hanmer 2000
   Angle 8 ; At last - my own Dragon curve variation (I HOPE)
   Axiom -x
   x=x+f+y-f
   y=f+x-f-y
}

ADH129a { ; Anthony Hanmer 2000
   Angle 8
   Axiom -x--x--x--x
   x=x+f+y-f-x
   y=y+f+x-f-y
}

ADH130 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=ff-f-f+f+ff
}

ADH130a { ; Anthony Hanmer 2000
   Angle 4
   Axiom f
   f=f+f-ff-f-f+f
}

ADH131 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f-ff+ff
}

ADH131a { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f-ff+ff
}

ADH132a { ; Anthony Hanmer 2000
   Angle 3 ; Inside-out Sierpinski
   Axiom f-f-f
   f=f+[f+g]-f
}

ADH132b { ; Anthony Hanmer 2000
   Angle 3 ; ALMOST Sierpinski
   Axiom f+f+f
   f=f+[@.8f+g]-f
}

ADH135 { ; Anthony Hanmer 2000
   Angle 8
   Axiom f++f++f++f
   f=fff+++fff-ffff+fff---fff
}

ADH135a { ; Anthony Hanmer 2000
   Angle 8
   Axiom [f-f+f]++[f-f+f]++[f-f+f]++[f-f+f]
   f=fff+++fff-ffff+fff---fff
}
  
ADH119 { ; Anthony Hanmer 2000
   Angle 4 ;  The one that started it all...
   Axiom f
   f=f+ff-f+f-ff-f
}

ADH119d { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-g+f-ff-f
}

ADH119e { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-f+g-ff-f
}

ADH119f { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-f+f-gf-f
}

ADH119g { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-f+f-fg-f
}

ADH119h { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-f+f-ff-g
}

ADH119i { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=g+ff-g+f-ff-g
}

ADH119j { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f
   f=f+ff-g+f-ff-g
}

ADH119k { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f
   f=f+ff-g+g-ff-f
}

ADH119m { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f
   f=g+ff-g+g-ff-g
}

ADH119n { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f
   f=f+gg-f+f-gg-f
}

ADH119o { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=+ff-f+ff-f-f
}

ADH119p { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f+fff-f+f-fff-f
}

ADH119s { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=ff-f+f-ff-f
}

ADH119v { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f
   f=ff+ff-f+f-ff-ff
}

ADH119w { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=-ff-f+f-ff-f
}

ADH119x { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f
   f=f-ff-f+f+ff-f
}

ADH119y { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=-ff-f+f-ff+f
}

ADH119z { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f+ff+f+f-ff-f
}

ADH119z1 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f--f--f--f
   f=f+ff-f+f-ff-f
}

ADH119z2 { ; Anthony Hanmer 2000
   Angle 4 ; ...er, L-system Julia, anyone...?
   Axiom f--f
   f=h+hh-h+h-hh-h
   f=f-ff+f-f+ff+f
}

ADH119z3 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=h+hh-h+h-hh-h
   f=f-ff+f-f+ff+f
}

ADH119za { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=+ff+f-f-ff+f
}

ADH119zb { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff+f-f+ff+f
}

ADH119zc { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=+ff+f-f-ff-f
}

ADH119zd { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=fg-f+gf-
}

ADH119ze { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff-f-f+ff-f
}

ADH119zf { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f
   f=f+ff-f+f+ff+f
}

ADH119zg { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++ff-f+f-ff-f
}

ADH119zh { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++gf-f+g-ff-f
}

ADH119zi { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++gf-f+f-ff-f
}

ADH119zj { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++fg-f+f-ff-f
}

ADH119zk { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff-g+f-ff-f
}

ADH119zl { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff-f+g-ff-f
}

ADH119zm { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff-f+f-gf-f
}

ADH119zn { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff-f+f-fg-f
}

ADH119zp { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=g++ff-g+f-ff-g
}

ADH119zq { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++ff-g+f-ff-g
}

ADH119zr { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++ff-g+g-ff-f
}

ADH119zs { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++gg-f+f-ff-f
}

ADH119zu { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++gg-f+f-gg-f
}

ADH119zv { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=+gf--f+gf-g-f
}

ADH119zw { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++fff-f+f-fff-f
}

ADH119zx { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++fg-f+g--g-fg-f
}

ADH119zy { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f++ff-f-f-ff-f
}

ADH119zza { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff-f+f-ff
}

ADH119zzb { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f
   f=fg--f+g-ff+
}

ADH119zzc { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=ff++ff-f+f-ff-ff
}

ADH119zzd { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=-fg--g+f--gg-f
}

ADH119zze { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f-fg--f+f+gf-f
}

ADH119zzg { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f+ff++f+f-ff-f
}

ADH119zzh { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=+ff++f-f-ff+f
}

ADH119zzi { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f++ff+f-f+ff+f
}

ADH119zzj { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=+ff++f-f-ff-f
}

ADH119zzk { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=fg--f+gf-
}

ADH119zzl { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+gg--f-g+fg-f
}

ADH119zzm { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+ff--f+f+ff+f
}

ADH119zzn { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f
   f=f+[f+g]-f
}

ADH119zzn1 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f
   f=f+[f+g]-f
}

ADH119zzn2 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f-f-f-f+f-f-f-f+f-f-f-f+f-f-f-f
   f=f+[f+g]-f
}

ADH119zzn3 { ; Anthony Hanmer 2000
   Angle 4
   Axiom f+f+f+f-f+f+f+f-f+f+f+f-f+f+f+f
   f=f+[f+g]-f
}

ADH120a { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f+f-ff-f
}

ADH120b { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+gg-f+g-gg-f
}

ADH120c { ; Anthony Hanmer 2000
   Angle 3 ; Sierpinski variation...
   Axiom f-f-f
   f=f+fg-f+g-ff-f
}

ADH120d { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-g+g-f
}

ADH120e { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f+g-ff-f
}

ADH120f { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f+f-gf-f
}

ADH120h { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f+f-ff-g
}

ADH120i { ; Anthony Hanmer 2000
   Angle 3
   Axiom f
   f=g+ff-g+f-ff-g
}

ADH120j { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+gf-g+f-gf-g
}

ADH120k { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-g+g-ff-f
}

ADH120l { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+gg-f+f-ff-f
}

ADH120o { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=+ff-f+ff-f-f
}

ADH120p { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+fff-f+f-fff-f
}

ADH120q { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+ff-f+f-f-ff-f
}

ADH120r { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+ff-f-f-ff-f
}

ADH120s { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=ff-f+f-ff-f
}

ADH120t { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f+f-ff
}

ADH120u { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=ff-f+f-ff+
}

ADH120v { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=ff+ff-f+f-ff-ff
}

ADH120x { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-ff-f+f+ff-f
}

ADH120y { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=-ff-f+f-ff+f
}

ADH120z { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+[gg+f]+g-
}

ADH120zb { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff+f-[f+f]f+f
}

ADH120zd { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=fg-f+gf-
}

ADH120ze { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-f-f+ff-f
}

ADH120zf { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+ff-[f+f]+ff+f
}

ADH120zg { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=[f-ff]-f+f-ff-f
}

ADH120zh { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-gg-f+g-gg-f
}

ADH120zi { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-[gfg]-g+f-gg-f
}

ADH120zj { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=g-fg-g-gg-f
}

ADH120zm { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-ff-f+f-[gf-f]
}

ADH120zn { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-g[f-g+f]-gg-f
}

ADH120zo { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-gg+f-ff-g
}

ADH120zp { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=g-ff-g+f-ff-g
}

ADH120zq { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=g-ff-g+f-ff-g
}

ADH120zr { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f-gf-g+f-gg-f
}

ADH120zs { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f-gg-f+g-gf-f
}

ADH120zt { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=g-ff-g+g-ff-g
}

ADH120zx { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-fg-f+g+g-fg-f
}

ADH120zy { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f-ff-f-f-ff-f
}

ADH120zz { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=ff+f+g-ff-f
}

ADH120zza { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-ff-f+f-ff
}

ADH120zzb { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=fg+f+g-fg+
}

ADH120zze { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f-fg+f+f+gf-f
}

ADH120zzf { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=-fg+f+g-fg-f
}

ADH120zzg { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=f+ff-f+f-ff-f
}

ADH120zzh { ; Anthony Hanmer 2000
   Angle 3
   Axiom f+f+f
   f=+ff-f-f-ff+f
}

ADH120zzi { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f-ff+f-f+ff+f
}

ADH120zzj { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=+ff-f-f-ff-f
}

ADH120zzl { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+gg+f-g+fg-f
}

ADH120zzm { ; Anthony Hanmer 2000
   Angle 3
   Axiom f-f-f
   f=f+fg+f+f+gg+f
}

ADH121f { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+ff-f+f-gf-f
}

ADH121g { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+ff-f+f-fg-f
}

ADH121h { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+ff-f+f-ff-g
}

ADH121i { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=g+ff-g+f-ff-g
}

ADH121m { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=g+ff-g+g-ff-g
}

ADH121n { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+gg-f+f-gg-f
}

ADH121p { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+fff-f+f-fff-f
}

ADH121v { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=ff+ff-f+f-ff-ff
}

ADH121x { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f-ff-f+f+ff-f
}

ADH121y { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=-ff-f+f-ff+f
}

ADH121z { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=f+ff+f+f-ff-f
}

ADH121zd { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=fg-f+gf-
}

ADH121ze { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+ff-f-f+ff-f
}

ADH121zg { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=f++ff-f+f-ff-f
}

ADH121zk { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f++ff-g+f-ff-f
}

ADH121zs { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=f++gg-f+f-ff-f
}

ADH121zu { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f++gg-f+f-gg-f
}

ADH121zv { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=+gf--f+gf-g-f
}

ADH121zza { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f++ff-f+f-ff
}

ADH121zzb { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=fg--f+g-ff+
}

ADH121zzc { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=ff++ff-f+f-ff-ff
}

ADH121zzd { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=-fg--g+f--gg-f
}

ADH121zze { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f-fg--f+f+gf-f
}

ADH121zzf { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=-fg--f+g-fg++f
}

ADH121zzh { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=+ff++f-f-ff+f
}

ADH121zzk { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=fg--f+gf-
}

ADH121zzl { ; Anthony Hanmer 2000
   Angle 5
   Axiom f-f-f-f-f
   f=f+gg--f-g+fg-f
}

ADH121zzn { ; Anthony Hanmer 2000
   Angle 5
   Axiom f+f+f+f+f
   f=f+[f+g]-f
}


ADH122a { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+ff-f+f-ff-f
}

ADH122b { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+gf-f+f-ff-f
}

ADH122d { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+ff-g+f-ff-f
}

ADH122e { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+ff-f+g-ff-f
}

ADH122f { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+gg-f+f-gf-f
}

ADH122i { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=g+ff-g+f-ff-g
}

ADH122j { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+ff-g+f-ff-g
}

ADH122k { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+ff-g+g-ff-f
}

ADH122l { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+gg-f+f-ff-f
}

ADH122m { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=g+[ff-gf]-fg
}

ADH122n { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+gg-f+f-gg-f
}

ADH122o { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=+ff-f+ff-f-f
}

ADH122p { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f
   f=f+fff-f+f-fff-f
}

ADH122q { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f+ff-f+f-f-ff-f
}

ADH122s { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=ff-f+f-ff-f
}

ADH122v { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f
   f=ff+ff-f+f-ff-ff
}

ADH122w { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=-ff-f+f-ff-f
}

ADH122x { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f-ff-f+f+ff-f
}

ADH122y { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=-ff-f+f-ff+f
}

ADH122z { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f
   f=f+ff+f+f-ff-f
}

ADH122za { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=+ff+f-f-ff+f
}

ADH122zb { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+ff+f-f+ff+f
}

ADH122zc { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=+ff+f-f-ff-f
}

ADH122zd { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=fg-f+gf-
}

ADH122ze { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+ff-g-f+ff-f
}

ADH122zf { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f+ff-f+f+ff+f
}

ADH122zg { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f++ff-f+f-ff-f
}

ADH122zi { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f++gf-f+f-ff-f
}

ADH122zm { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f++ff-f+f-gf-f
}

ADH122zr { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f++ff-g+g-ff-f
}

ADH122zs { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f++gg-f+f-ff-f
}

ADH122zv { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=+gf[-f+gf]-g
}

ADH122zx { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f++[fg-f+g]--g-fg-f
}

ADH122zy { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f++ff-f-f-ff-f
}

ADH122zz { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f[f--f+g]+f-f
}

ADH122zza { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f
   f=f++ff-f+f-ff
}

ADH122zze { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f
   f=f-fg--f+f+gf-f
}

ADH122zzf { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=-[fg-]fg+g-[-fg+]+ff
}

ADH122zzg { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=gg+[fg+]-f
}

ADH122zzh { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=+fg++f-f-gg+f
}

ADH122zzi { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f++ff+f-f+ff+f
}

ADH122zzj1 { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f+[f+g]-f
}

ADH122zzj2 { ; Anthony Hanmer 2000
   Angle 6
   Axiom f++f++f
   f=f+[f+g]-f
}

ADH122zzk { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=fg--f+gf-
}

ADH122zzl { ; Anthony Hanmer 2000
   Angle 6
   Axiom f+f+f+f+f+f
   f=f+[gg-f+f]-f-g+
}

ADH122zzm { ; Anthony Hanmer 2000
   Angle 6
   Axiom f-f-f-f-f-f
   f=f+ff--f+f+ff+f
}

ADH123b { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f+gf-f+f-ff-f
}

ADH123c { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=f+fg-f+f-ff-f
}

ADH123d { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f+ff-g+f-ff-f
}

ADH123e { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f+ff-f+g-ff-f
}

ADH123f { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f+ff-f+f-gf-f
}

ADH123h { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f+ff-f+f-ff-g
}

ADH123m { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=g+ff-g+g-ff-g
}

ADH123o { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=+ff-f+ff-f-f
}

ADH123t { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f[+gf-[g+f]-g]+f
}

ADH123u { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=ff-[f+f]-gg+
}

ADH123v { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f
   f=ff+ff-f+f-ff-ff
}

ADH123w { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=-ff-f+f-ff-f
}

ADH123y { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=-ff-f+f-ff+f
}

ADH123z { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f
   f=f+ff+f+f-ff-f
}

ADH123zb { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=f+[gg-[f-f]+fg]+f
}

ADH123zc { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=+ff+f-f-ff-f
}

ADH123zd { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=fg-f+gf-
}

ADH123ze { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=f+gg-f-[g+f]g
}

ADH123ze1 { ; Anthony Hanmer 2000
   Angle 8
   Axiom f--f--f--f--f--f--f--f
   f=f+gg-f-[g+f]g
}

ADH123ze2 { ; Anthony Hanmer 2000
   Angle 8
   Axiom f
   f=f+gg-f-[g+f]g
}

ADH123zt { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f-g+g-ff-g
}

ADH123zu { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f++gg-f+f-gg-f
}

ADH123zv { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=+gf--f+gf-g-f
}

ADH123zw { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=f+fgf-f+g+fgf-f
}

ADH123zx { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f++fg-f+g--g-fg-f
}

ADH123zza { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=f++ff-f+f-ff
}

ADH123zzc { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=ff+[+ff-f+]f-ff-ff
}

ADH123zzf { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f
   f=-fg--f+g-fg++f
}

ADH123zzg { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=g+ff++g+f-gg-f
}

ADH123zzh { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=+ff++f-f-ff+f
}

ADH123zzk { ; Anthony Hanmer 2000
   Angle 8
   Axiom f-f-f-f-f-f-f-f
   f=fg--f+gf-
}

ADH123zzn { ; Anthony Hanmer 2000
   Angle 8
   Axiom f+f+f+f+f+f+f+f
   f=f+[f+g]-f
}

ADH123zzn1 { ; Anthony Hanmer 2000
   Angle 8
   Axiom f++f++f++f
   f=f+[@.9f+g]-f
}
