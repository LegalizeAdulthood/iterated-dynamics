.. _summary-of-commands:

Summary of Commands
===================

Hit any of these keys at the menu or while drawing or viewing a fractal.
Commands marked with an '*' are also available at the credits screen.

:ref:`plotting-commands`
 * Delete,F2,F3,.. Select a Video Mode and draw (or redraw) current fractal
 * F1              HELP! (Enter help mode)
   Esc or m        Go to main menu
   h               Redraw previous screen (you can 'back out' recursively)
   Ctrl-H          Redraw next screen in history circular buffer
   Tab             Display information about the current fractal image
 * t               Select a new fractal type and parameters
 * x               Set a number of options and doodads
 * y               Set extended options and doodads
 * z               Set fractal type-specific parameters
   p               Set passes options
   c or + or -     Enter Color-Cycling Mode (see :ref:`color-cycling-commands`)
   e               Enter Palette-Editing Mode (see :ref:`palette-editing-commands`)
   Spacebar        Mandelbrot/Julia Set toggle.
   Enter           Continue an interrupted calculation (e.g. after a save)
 * f               toggle the floating-point algorithm option ON or OFF
 * i               Set parameters for 3D fractal types
 * Insert          Restart the program (at the credits screen)
   a               Convert the current image into a fractal 'starfield'
   Ctrl-A          Turn on screen-eating ant automaton
   Ctrl-S          Convert current image to a Random Dot Stereogram (RDS)
   o               toggles 'orbits' option on and off during image generation
 * d               Shell to DOS (type 'exit' at the DOS prompt to return)
   Ctrl-X          Flip the current image along the screen's X-axis
   Ctrl-Y          Flip the current image along the screen's Y-axis
   Ctrl-Z          Flip the current image along the screen's Origin

:ref:`image-save-restore-commands`
   s               Save the current screen image to disk
 * r               Restore a saved (or .GIF) image ('3' or 'o' for 3-D)

:ref:`orbits-window
   o               Turns on Orbits Window mode after image generation
   ctrl-o          Turns on Orbits Window mode

:ref:`view-window`
 * v               Set view window parameters (reduction, aspect ratio)

:ref:`print-command`
   ctrl-p          Print the screen (command-line options set printer type)

:ref:`parameter-save-restore-commands`
   b               Save commands describing the current image in a file
                   (writes an entry to be used with @ command)
 * @ or 2          Run a set of commands (in command line format) from a file
   g               Give a startup parameter: :ref:`summary-of-all-parameters`

:ref:`3d-commands`
 * 3               3D transform a saved (or .GIF) image
   # (shift-3)     same as 3, but overlay the current image

:ref:`zoom-box-commands`
   PageUp          When no Zoom Box is active, bring one up
                   When active already, shrink it
   PageDown        Expand the Zoom Box
                   Expanding past the screen size cancels the Zoom Box
   \24 \25 \27 \26         Pan (Move) the Zoom Box
   Ctrl- \24 \25 \27 \26   Fast-Pan the Zoom Box (may require an enhanced keyboard)
   Enter           Redraw the Screen or area inside the Zoom Box
   Ctrl-Enter      'Zoom-out' - expands the image so that your current
                   image is positioned inside the current zoom-box location.
   Ctrl-Pad+/Pad-  Rotate the Zoom Box
   Ctrl-PgUp/PgDn  Change Zoom Box vertical size (change its aspect ratio)
   Ctrl-Home/End   Change Zoom Box shape
   Ctrl-Ins/Del    Change Zoom Box color

:ref:`interrupting-and-resuming`

:ref:`video-mode-function-keys`

:ref:`browse-commands`
  L(ook)                   Enter Browsing Mode

:ref:`evolver-commands`
  Ctrl-E                   Bring up :ref:`explorer-evolver` control screen
  Alt-1 ... Alt-7          Enter evolver mode with selected level of
                           mutation: Alt-1 = low level, Alt-7 = maximum.
                           (dont use the keypad, just the 'top row' numbers)
                           When in evolve mode then just plain 1..7 also work

:ref:`rds-commands`
  Ctrl-S                   Access RDS parameter screen
