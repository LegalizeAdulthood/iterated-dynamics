// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "ui/ant.h"

#include "ui/AntController.h"

using namespace id::engine;

namespace id::ui
{

// TODO: removed later
// Slice 52.12 replaces the synchronous entry with outer UI stepping.
int ant_type(NonStandardFractal &calculation)
{
    AntController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    return controller.result();
}

} // namespace id::ui
