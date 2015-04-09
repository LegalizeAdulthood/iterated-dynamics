.. _plotting-commands:

Plotting Commands
=================

Function keys & various combinations are used to select a video mode and
redraw the screen.  For a quick start try one of the following:

  If you have MCGA, VGA, or better:  <F3>
  If you have EGA:                   <F9>
  If you have CGA:                   <F5>
  Otherwise, monochrome:             <F6>

<F1>
  Display a help screen. The function keys available in help mode are
  displayed at the bottom of the help screen.

<M> or <Esc>
  Return from a displayed image to the main menu.

<Esc>
  From the main menu, <Esc> is used to exit from Fractint.

<Delete>
  Same as choosing "select video mode" from the main menu.
  Goes to the "select video mode" screen.  See {Video Mode Function Keys}.

<h>
  Redraw the previous image in the circular history buffer, revisiting fractals
  you previously generated this session in reverse order. Fractint saves
  the last ten images worth of information including fractal type, coordinates,
  colors, and all options. Image information is saved only when some item
  changes. After ten images the circular buffer wraps around and earlier
  information is overwritten. You can set image capacity of the history feature
  using the maxhistory=<nnn> command. About 1200 bytes of memory is required
  for each image slot.

<Ctrl-h>\
Redraw the next image in the circular history buffer. Use this to return to
images you passed by when using <h>.

<Tab>\
Display the current fractal type, parameters, video mode, screen or (if
displayed) zoom-box coordinates, maximum iteration count, and other
information useful in keeping track of where you are.  The Tab function is
non-destructive - if you press it while in the midst of generating an
image, you will continue generating it when you return.  The Tab function
tells you if your image is still being generated or has finished - a handy
feature for those overnight, 1024x768 resolution fractal images.  If the
image is incomplete, it also tells you whether it can be interrupted and
resumed.  (Any function other than <Tab> and <F1> counts as an
"interrupt".)

The Tab screen also includes a pixel-counting function, which will count
the number of pixels colored in the inside color.  This gives an estimate
of the area of the fractal.  Note that the inside color must be different
from the outside color(s) for this to work; inside=0 is a good choice.

<T>\
Select a fractal type. Move the cursor to your choice (or type the first
few letters of its name) and hit <Enter>. Next you will be prompted for
any parameters used by the selected type - hit <Enter> for the defaults.
See {Fractal Types} for a list of supported types.

<F>\
Toggles the use of floating-point algorithms
(see {"Limitations of Integer Math (And How We Cope)"}).
Whether floating point is in
use is shown on the <Tab> status screen.  The floating point option can
also be turned on and off using the "X" options screen.
If you have a non-Intel floating point chip which supports the full 387
instruction set, see the "FPU=" command in {Startup Parameters}
to get the most out of your chip.

<X>\
Select a number of eXtended options. Brings up a full-screen menu of
options, any of which you can change at will.  These options are:\

  "passes=" - see {Drawing Method}\
  Floating point toggle - see <F> key description below\
  "maxiter=" - see {Image Calculation Parameters}\
  "inside=" and "outside=" - see {Color Parameters}\
  "savename=" filename - see {File Parameters}\
  "overwrite=" option - see {File Parameters}\
  "sound=" option - see {Sound Parameters}\
  "logmap=" - see {Logarithmic Palettes and Color Ranges}\
  "biomorph=" - see {Biomorphs}\
  "decomp=" - see {Decomposition}\
  "fillcolor=" - see {Drawing Method}\

<Y>\
More options which we couldn't fit under the <X> command:\

  "finattract=" - see {Finite Attractors}\
  "potential=" parameters - see {Continuous Potential}\
  "invert=" parameters - see {Inversion}\
  "distest=" parameters - see {Distance Estimator Method}\
  "cyclerange=" - see {Color Cycling Commands}\

<P>\
Options that apply to the Passes feature:\

  "periodicity=" - see {Periodicity Logic}\
  "orbitdelay=" - see {Passes Parameters}\
  "orbitinterval=" - see {Passes Parameters}\
  "screencoords=" - see {Passes Parameters}\
  "orbitdrawmode=" - see {Passes Parameters}\

<Z>\
Modify the parameters specific to the currently selected fractal type.
This command lets you modify the parameters which are requested when you
select a new fractal type with the <T> command, without having to repeat
that selection. You can enter "e" or "p" in column one of the input fields
to get the numbers e and pi (2.71828... and 3.14159...).\
From the fractal parameters screen, you can press <F6> to bring up a
sub parameter screen for the coordinates of the image's corners.
With selected fractal types, <Z> allows you to change the {Bailout Test}.
; With the IFS fractal type, <Z> brings up the IFS editor (see
; {=HT_IFS Barnsley IFS Fractals}).

<+> or <->\
Switch to color-cycling mode and begin cycling the palette
by shifting each color to the next "contour."  See {Color Cycling Commands}.\

<C>\
Switch to color-cycling mode but do not start cycling.
The normally black "overscan" border of the screen changes to white.
See {Color Cycling Commands}.

<E>\
Enter Palette-Editing Mode.  See {Palette Editing Commands}.

<Spacebar>\
Toggle between Mandelbrot set images and their corresponding Julia-set
images. Read the notes in {=HT_JULIA Fractal Types, Julia Sets}
before trying this option if you want to see anything interesting.

<J>\
Toggle between Julia escape time fractal and the Inverse Julia orbit
fractal. See {=HT_INVERSE Inverse Julias}

<Enter>\
Enter is used to resume calculation after a pause. It is only
necessary to do this when there is a message on the screen waiting to be
acknowledged, such as the message shown after you save an image to disk.

<I>\
Modify 3D transformation parameters used with 3D fractal types such as
"Lorenz3D" and 3D "IFS" definitions, including the selection of
{=HELP3DGLASSES "funny glasses"} red/blue 3D.

<A>\
Convert the current image into a fractal 'starfield'.  See {Starfields}.

<Ctrl-A>\
Unleash an image-eating ant automaton on current image. See {Ant Automaton}.

<Ctrl-S> (or <k>)\
Convert the current image into a Random Dot Stereogram (RDS).
See {Random Dot Stereograms (RDS)}.

<O> (the letter, not the number)\
If pressed while an image is being generated, toggles the display of
intermediate results -- the "orbits" Fractint uses as it calculates values
for each point. Slows the display a bit, but shows you how clever the
program is behind the scenes. (See "A Little Code" in
{"Fractals and the PC"}.)

<D>\
Shell to DOS. Return to Fractint by entering "exit" at a DOS prompt.

<Insert>\
Restart at the "credits" screen and reset most variables to their initial
state.  Variables which are not reset are: savename, lightname, video,
startup filename.

<L>\
Enter Browsing Mode.  See {Browse Commands}.

<Ctrl-E>\
Enter Explorer/Evolver Mode.  See {Evolver Commands}.
