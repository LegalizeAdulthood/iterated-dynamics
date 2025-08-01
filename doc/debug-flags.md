# Debug Flags

This list is complete as of Iterated Dynamics version 1.0.
None of these are necessarily supported in future.

Add one to any debug value to trigger writing benchmark values to
the file `id-bench.txt`.  This gets stripped at startup; so all values
used for other purposes are even.

Example:
`id debug=22` forces Id to use floating-point math for 3D perpsective calculations.
`id debug=23` does the same thing, but also turns on the benchmark timer.

| Value | Meaning |
|:--:|----|
| 2 | append image history values as JSON to history.json |
| 22 | force float for 3D perspective |
| 50 | compare <r>estored files |
| 90 | force "C" mandel & julia code (no calcmand or calmanfp) |
| 90 | force generic code for fn+fn etc types |
| 90 | force "C" parser code even if FPU >= 387 |
| 96 | Use real formula for popcorn in the old case. |
| 96 | write debug messages to disk files(3) |
| 98 | write formula parser tokens to frmtokens.txt |
| 110 | turns off first-time initialization of variables |
| 200 | time encoder |
| 300 | prevent modified inverse iteration method (MIIM) |
| 324 | disables help ESC in screen messages |
| 420 | don't use extended/expanded mem (force disk) same for screen save (force disk) |
| 422 | don't use expanded mem (force extended or disk)  same for screen save (force extended or disk) |
| 470 | disable prevention of color 0 for BTM |
| 472 | enable solid guessing at bottom and right |
| 7nn | set `<b>` getprec() digits variable to nn |
| 750 | print out as many params digits as possible in PAR |
| 910 | disables Sylvie Gallet's colors= color compression fix. |
| 920 | makes colors= compression lossless. |
| 1010 | force fp for newton & newtbasin (no mpc math) |
| 1012 | swap sign bit of zero for mandelbrotmix4 |
| 1234 | force larger integer arithmetic bitshift value |
| 2224 | old just-the-dots logic, probably a temporary thing |
| 3200 | disable auto switch back from arbitrary precision |
| 3400 | disable auto switch from integer to float |
| 3600 | pins the plasma corners to 1 |
| 3800 | turns off grid pixel lookup |
| 4010 | use old centermag conversion. |
| 4030 | use old orbit->sound code w/integer overflow. |
| 4200 | sets disk video cache size to minimum |
| 6000 | turns off optimization of using realzzpower types instead of complexzpower when imaginary part of parameter is zero |
| 10000 | display cpu, fpu, and free memory at startup |
| `!=0` | show "normal vector" errors instead of just fixing |
| `!=0` | show info if fullscreen_prompt array invalid |
