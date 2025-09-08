// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#if defined(ID_VERBOSE)
#define ODS(text_)                      ::id::misc::ods(__FILE__, __LINE__, text_)
#define ODS1(fmt_, arg_)                ::id::misc::ods(__FILE__, __LINE__, fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)            ::id::misc::ods(__FILE__, __LINE__, fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)       ::id::misc::ods(__FILE__, __LINE__, fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)      ::id::misc::ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)  ::id::misc::ods(__FILE__, __LINE__, fmt_, _1, _2, _3, _4, _5)
namespace id::misc
{

void ods(char const *file, unsigned int line, char const *format, ...);

} // namespace id::misc
#else
#define ODS(text_)
#define ODS1(fmt_, arg_)
#define ODS2(fmt_, a1_, a2_)
#define ODS3(fmt_, a1_, a2_, a3_)
#define ODS4(fmt_, _1, _2, _3, _4)
#define ODS5(fmt_, _1, _2, _3, _4, _5)
#endif
