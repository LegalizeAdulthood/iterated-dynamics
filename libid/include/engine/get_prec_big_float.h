// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/big.h"

namespace id
{

int get_prec_bf(math::ResolutionFlag flag);
int get_prec_bf_mag();
int get_magnification_precision(LDouble magnification);

} // namespace id
