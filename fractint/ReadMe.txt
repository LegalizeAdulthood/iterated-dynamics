Project Structure

common		Files common to all implementations
	3d
		Files containing 3D drawing support.  Eventually these should be
		subsumed into the driver interface, with this sofwtare implementation
		as a fallback if the driver doesn't support 3D rendering.
	
	engine
		Files containing the implementation of the fractal rendering engines
		that are shared between multiple fractal types.  Also contains the
		big array that defines the available fractal types.
	
	fractal specific
		Files containing rendering code or user interaction code that is
		specific to a particular fractal, such as the Lorenz fractal or
		L-system type.
	
	i/o
		Anything related to doing external file I/O: saving and loading GIF
		files, dealing with overlay files, saving parameter files, etc.
		
	math
		Files containing math routines for arbitrary precision arithmetic,
		complex arithmetic, etc.

	plumbing
		Miscellaneous routines that don't fit elesewhere like memory and
		driver management.

	ui
		Anything to do with displaying menu screens, help screens, intro
		screen, etc.

data
	All the data files that go along with a complete fractint installation
	but aren't needed for compiling.  These are kept here for reference and
	so that they can be edited directly from VS.NET.

drivers
	Files specific to each driver.  Generally these are named d_XXX.c, but
	if you have a lot of files in your driver implementation, add a folder
	here called d_XXX and put all your driver files in that folder, both
	on disk and in the project.
	
headers
	Header files

help
	Help source files with a custom build step on help.src to run the help
	compiler on the source to generate new FRACTINT.HLP and HELPDEFS.H
	files.

main
	Miscellaneous files in the main fractint source folder.  These are not
	currently used for any of the compilation of the code and are placed
	here just for reference.

win32
	Files needed for the Win32 platform.

TODO:
	- Windows printing
	- GDI based driver
	- DirectX based driver
	- Get key reading semantics right

2006.12.26.23.09
	Basic plotting through dotwrite working.
2006.12.27.00.53
	vidtbl vs. g_video_table confusion resolved.
	Eliminate one use of extraseg for vidtbl.
2006.12.27.02.26
	Basic disk video messages displaying.
2006.12.28.17.34
	Disk video status screen corrected.
	Memory error in disk cache corrected.
	TAB display showing properly.
	Add driver name and description to TAB screen.
	Deprecate fracting.cfg for video modes.
2006.12.31.12.53
	Fix bug with video table when you select a video mode the 2nd time around.
	Fix space toggle bug on intro screen, but still too many keyboard routines;
	need to narrow it down to just the two driver routines and not all these
	inner layers.
2007.01.01.12.24
	Fix expand_dirname problem.
2007.01.01.21.17
	Key input working reliably and consistently now!  Whew!
	Ctrl+TAB bug fixed.  F10 and other "system key down" events reported.
