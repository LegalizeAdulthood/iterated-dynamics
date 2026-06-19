# Reporting Problems

While every effort has been made to ensure that this release is free of
problems, using both automated and manual testing, if you encounter a
problem, please open an
[issue on github](https://github.com/LegalizeAdulthood/iterated-dynamics/issues).

There are some known bugs, mostly with respect to different renderings of
[Fractal of the Day images](https://user.xmission.com/~legalize/fractals/fotd/).
The documentation lists known limitations of this release.

The [release plan](https://github.com/LegalizeAdulthood/iterated-dynamics/wiki)
outlines in broad strokes the direction of future development.

# Dependencies

The Setup program should apply the necessary Visual C++ runtime if it is not
installed on your system.  If you encounter this problem and used the Setup
program, please
[file an issue](https://github.com/LegalizeAdulthood/iterated-dynamics/issues)
on that; the Setup program should have taken care of the dependency
automatically but it is difficult for us to test this since we already have
the dependency installed.

The standalone ZIP and MSI packages assume the runtime is already installed
on your machine.

If you get an error message about missing the following files:
- `MSVCP140.dll`
- `VCRUNTIME140.dll`
- `VCRUNTIME140_1.dll`

It means you don't have the Visual C++ runtime files installed on your
machine.  You can install them from here:

[https://aka.ms/vs/17/release/vc_redist.x64.exe](https://aka.ms/vs/17/release/vc_redist.x64.exe)

Make sure you install the `x64` (64-bit) version.
