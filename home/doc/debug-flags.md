# Debug Flags

`debugflag=nnn` sets developer debugging behavior.  `debug=nnn` is
accepted as a synonym.

| Value | Meaning |
|:--:|----|
| 2 | Append image history values as JSON to `history.json`. |
| 50 | Compare restored image pixels instead of normal restore. |
| 90 | Force standard or generic fractal calculation paths. |
| 96 | Force the real Popcorn orbit for the old default setup. |
| 98 | Write formula parser and interpreter debug files. |
| 110 | Allow startup-only commands after startup. |
| 300 | Prevent modified inverse iteration method (MIIM). |
| 324 | Leave formula compile information visible. |
| 420 | Force memory and screen-save allocation to disk. |
| 422 | Force memory and screen-save allocation to memory. |
| 470 | Allow boundary-trace color-zero behavior. |
| 472 | Enable solid guessing at bottom and right edges. |
| 700-719 | Set PAR numeric precision to value minus 700 digits. |
| 750 | Write params with long-double precision in PAR output. |
| 910 | Allow large color-map component jumps during compression. |
| 920 | Force lossless color-map compression. |
| 1012 | Flip MandelbrotMix4 zero sign handling. |
| 3200 | Force or keep arbitrary precision math where supported. |
| 3400 | Prevent automatic arbitrary precision math. |
| 3600 | Pin Plasma random table values to 1. |
| 4010 | Allow negative cross product in center-mag conversion. |
| 4030 | Use the old scaled orbit-to-sound formula. |
| 6000 | Force complex z-power math instead of real-power shortcut. |
| 10000 | Display memory allocation diagnostics and bounds checks. |
| `!=0` | Show extra debug messages in stop-message and 3D paths. |
