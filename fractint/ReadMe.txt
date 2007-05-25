Tip: In VS.NET 2003/2005, select the FractIntSetup project in the Solution
explorer.  Then select Project / Unload Project to unload the setup
project.  It isn't built in the Debug configuration, but processing the
project itself is slow, so keep it unloaded until you need to build a
setup.

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
2007.01.01.23.38
	It renders!
2007.01.04.03.12
	Fix file/directory browsing bugs.

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

FRACTINT:
Fractint 20.4 complete C and ASM source for
the DOS-based fractal generator. Requires
Microsoft C/C++ 7.0 or later or Borland C/C++
3.1 or later. Object code of ASM modules 
supplied so an assembler is not required.

Note that the Borland project files are in the extra directory
and have not been updated to use the new source tree structure.

Copyrighted freeware.

XFRACTINT:
Download the source tarball (for example: xfract20.3.02.tar.gz), put it in
your home directory, and untar it with the command:

tar -xzf xfract20.3.02.tar.gz

This will create the directory xfractint-20.03p02 containing the source.
You might want to change the directory name to xfractint for convenience.

You will need to have gcc, ncurses, and ncurse-develop installed.  You also
need the XFree86-libs package installed for the X11 libraries.  This package
should already be installed, but if it isn't and your distribution doesn't
have it, then you need the XFree86-devel package.

The Makefile is set up for my convenience, so if you want to put files in
different directories, you will need to change the SRCDIR setting.  
Otherwise, just run make from the source directory and it should compile.  
Run ./xfractint to start it up.

Jonathan

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Visual C++ 2005 Express Edition
-------------------------------
Here are instructions for building FractInt for Windows using the free
Microsoft C++ express edition compiler.

1. Download source code and tools
2. Install source code and tools
3. Configure tools
4. Compile source
5. Write tests for new feature
6. Implement new feature until tests pass
7. goto 5 until done :-)

1. Download:

	To make changes to FractInt, you'll want to download the source code,
	and some development tools.  If you're reading this, its because you
	already have Microsoft Visual C++ 2005 Express Edition, or its because
	you don't have development tools and you need some free ones.

	FractInt for Windows:
		Download the FractInt for Windows source.  If you're reading this
		file, chances are you're reading it from a source distribution, so
		you already have the source code.  If not, go get it at
		<http://www.fractint.org>.
	
	Microsoft Visual C++ 2005 Express Edition:
		Download "Microsoft Visual C++ 2005 Express Edition" from
		Microsoft's web site.  Google for the above quoted phrase, or
		drill down from <http://msdn.microsoft.com>.
			
	Platform SDK:
		Visual C++ is just an ISO C++ compiler; it does not include headers
		or libraries for the Win32 API.  Those headers and libraries are in
		the Platform SDK, which you can download for free from Microsoft.

		The "Platform SDK", sometimes called the "Windows SDK", and
		sometimes called other things depending on what Microsoft decides.
		In reality it is an evolving SDK that contains documentation,
		headers and libraries for the latest-and-greatest version of the
		Win32 API.  The Win32 API tends to be updated with operating system
		releases and often includes the operating system name in the title.
		It might read "Windows Platform SDK Windows Server SP2 (Jun 2006)",
		which is telling you which OS features are enabled on that SDK and
		the date it was released.  You generally want the latest SDK since
		you're using the 2005 version of C++.
	
2. Installation

	a) Install Microsoft Visual C++ 2005 Express Edition
	b) Install the Platform SDK and make note of the install location
	c) Install the FractInt source and make note of the install location

3. Configuration

	a) Launch Visual C++
	b) Register it (for free), to get rid of the eventual nag boxes
	c) Configure Visual C++ directories:
		Visual C++ 2005 Express Edition didn't ship with the Platform SDK,
		so it doesn't know where to find the headers and libraries.  Select
		Tools / Options... and browse to the Projects and Solutions / VC++
		Directories category.  On the right, select the "Include files"
		item in the combo box, click the new directory icon and browse to
		the include files where you installed the Platform SDK.  Change
		the combo box to "Library files" and do the same for the library
		folder in the Platform SDK.  Change any other settings in the
		dialog to suit your tastes and click OK to save the changes.

4. Compilation

	Launch Visual C++ and select File / Open / Project/Solution... and
	browse to the fractint.sln solution file where you installed the
	FractInt for Windows source code.  The code should compile with no
	warnings or errors.  If you got any errors, particularly on files in
	the Win32 folder, then check your Platform SDK include and link
	directory settings.

5. Write tests for new feature

	When you're ready to add new features to FractInt for Windows,
	please consider a test-driven development methodology and write the
	tests for the changes you're about to make before you make the changes.
	I know that sounds silly, but its really what you do :-).  For more
	information read any of these books:
		"Test-Driven Development", Kent Beck
			ISBN-10: 0321146530
		"Working Effectively with Legacy Code", Michael Feathers
			ISBN-10: 0131177052			
		"Refactoring: Improving the Design of Existing Code", Martin Fowler
			ISBN-10: 0201485672

6. Implement feature until tests pass

	See, you wrote the test first so that it would tell you when your
	feature was done!  Once your tests pass, you'll know when you've done
	everything you wanted to do.
	
7. Goto 5 until no more features :-)

8. Submit changes back to the FractInt development team and enjoy your
	new-found fame.
