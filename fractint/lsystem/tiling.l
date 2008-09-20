
; These LSystems are designed to be run from FRACTINT.  No compilation is
; necessary, just copy this file to the FRACTINT directory and run FRACTINT.
; Select the lsystem fractal type and press F6 to select a file.


; This is the old ColorPenrose1 with three changes.
;   1. A brighter red and green
;   2. The name was changed to group the Penrose tilings together
;   3. The comment was corrected.

Penrose1Color { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
; Roger Penrose's rhombuses
; Uses color to show similar lines
  Angle 10
  Axiom +WC10F--XC12F---YC12F--ZC10F
  W=YC12F++ZC10F----XC12F[-YC12F----WC10F]++
  X=+YC12F--ZC10F[---WC10F--XC12F]+
  Y=-WC10F++XC12F[+++YC12F++ZC10F]-
  Z=--YC12F++++WC10F[+ZC10F++++XC12F]--XC12F
  F=
  }

; This tiling actually does what the old comment for ColorPenrose1 said
; it did.

Penrose1Forced { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
; Roger Penrose's rhombuses
; Uses color to show the edge matching rules to force nonperiodicy
  Angle 10
  Axiom +WC10FC12F--XC11FC10F---YC10FC11F--ZC12FC10F
  W=YC10FC11F++ZC12FC10F----XC11FC10F[-YC10FC11F----WC10FC12F]++
  X=+YC10FC11F--ZC12FC10F[---WC10FC12F--XC11FC10F]+
  Y=-WC10FC12F++XC11FC10F[+++YC10FC11F++ZC12FC10F]-
  Z=--YC10FC11F++++WC10FC12F[+ZC12FC10F++++XC11FC10F]--XC11FC10F
  F=
}

KitesAndDarts { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
; Roger Penrose's kites and darts
  Angle 10
  Axiom WG+XG+WG+XG+WG+XG+WG+XG+WG+X
  W=[F][++@1.618033989F][++G---@.618033989G|X-Y|G|W]
  X=[F+++@1.618033989F][++@.618033989GZ|X|-G|W]
  Y=[+F][@1.618033989F][+G@.618033989|Y+X]
  Z=[-F][@1.618033989F][@.618033989G--WG|+Z]
  F=
  }

KitesAndDartsColor { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
; Roger Penrose's kites and darts
  Angle 10
  Axiom WG+XG+WG+XG+WG+XG+WG+XG+WG+X
  W=[C10F][++@1.618033989C12F][++G---@.618033989G|X-Y|G|W]
  X=[C10F+++@1.618033989C12F][++@.618033989GZ|X|-G|W]
  Y=[+C10F][@1.618033989C12F][+G@.618033989|Y+X]
  Z=[-C10F][@1.618033989C12F][@.618033989G--WG|+Z]
  F=
  }

SpiralTile { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers"
  angle 16
  axiom X++X++X++X++|G|X++X++X++X
  X=[C12FX+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
  Y=[C12F+++++@.7653668647C10F@I.7653668647[-----Y]+++++C12F]
  }

VoderbergTile { ; by Herb Savage
; based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers",
; A spiral tiling by Heinz Voderberg
  angle 30
  axiom \84.1A\96@4.783386117M@I4.783386117/96A
  A=X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12X\12Z
  X=[D\78D\46.37236@3.393427D@I3.393427/46.37236D\114[\168X\24Y]D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D]
  Y=[D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D\168[\192Y]D\78D\46.37236@3.393427D@I3.393427/46.37236D]
  Z=[D\78D\46.37236@3.393427D@I3.393427/46.37236D\114D\78D\46.37236@3.393427D@I3.393427/46.37236D/78D]
  }

; These two aren't tilings but simple spiral patterns done by my 12 year old
; son Morgan that really need the warning from "Fractal Creations" about
; color cycling introducing hypnotic states.

Vertigo1 { ; by Morgan Savage
; Try order 13 and color cycle in both 256 and 16 color modes
  Angle 46
  Axiom X
  X=X>1F+@.9997X
  }

Vertigo2 { ; by Morgan Savage
; Try order 13 and color cycle in both 256 and 16 color modes
  Angle 49
  Axiom X
  X=X>1F+@.9997X
  }

