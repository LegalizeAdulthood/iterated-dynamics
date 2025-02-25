# Known Bugs

This file describes known issues in this release, from oldest to newest:

There is no sound output support.

Video modes with pixel dimensions other than 4/3 aspect ratio assume
non-square pixels.  The images all render fine, but they appear stretched
or squashed.  The choice of resolutions in id.cfg reflect this.

With `debugflag=10000`, error messages are reported for disk video mode when:
- start id
- pick any disk video mode (e.g. 320x200)
-  let it render
- wait for completion
- go to the <v> screen
- change 320 to 32
- submit it.
- This problem is present in the DOS fractint.

`<\>` or `<h>` or `<ctrl-h>` just redraws the current image instead of moving
backwards through the history buffer.

Does `savetime=` in parameter files work?
