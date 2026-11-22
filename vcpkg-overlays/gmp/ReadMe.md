<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# GMP Overlay

This copies the GMP 6.3.0 port, revision 2, from the pinned vcpkg baseline
`ce613c41372b23b1f51333815feb3edd87ef8a8b`. The upstream port and patches
retain their MIT license; see `LICENSE.txt`.

The pinned port requests Autoconf 2.71 package revision 3, which is no
longer available from the MSYS2 mirrors. This overlay changes only that
package URL and checksum to revision 4 and bumps the local port revision
to 3. The GMP source, build options, and upstream patches are unchanged.

The replacement is the published [MSYS2 Autoconf 2.71 package][autoconf].
Its SHA256 is
`55be2b911beaa0424d60fdcf206f2411a0eb24cb53e66a82964497ef8a4faac0`.
The port pins its SHA512 for vcpkg's normal download verification.

Remove the overlay when the registry baseline supplies the corrected
package reference.

[autoconf]: https://packages.msys2.org/packages/autoconf2.71
