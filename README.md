# Celtic Knots


Celtic Knots is a Windows screen saver I wrote in C++ some number of years ago.
The code hasn't been cleaned up at all, but I'm releasing it in the hope that
someone may find it interesting.

The code is released open-source under the GNU General Public License version 3.

The screen saver generates and animates random Celtic knots. These knots have
the property that each ribbon alternates between passing over and under each
ribbon it crosses. No ribbon will cross over twice or under twice in a row.
This is enforced using a novel grid-based knot generating algorithm.

The curves are calculated using [Cubic Hermite
splines](http://codeplea.com/introduction-to-splines). The file *spline.hpp* is
self-contained and may be of use to someone needing spline calculations. The
finial knot geometry is rendered using OpenGL.

# Demo

![celtic_knot demo](demo.gif)

# Building

The screen saver cross-compiles from Linux or WSL with MinGW-w64. On Debian or
Ubuntu:

    sudo apt install g++-mingw-w64-x86-64
    make

This produces a 64-bit `celtic_knots.scr`. On Windows with MinGW or MSYS2, run
`make CXX=g++ CC=gcc` instead. The file *third_party/scrnsave.c* is the
public-domain screen saver library from mingw-w64, compiled in directly because
some Linux distributions ship an empty 64-bit `libscrnsave.a`.
