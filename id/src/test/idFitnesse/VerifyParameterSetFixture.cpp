#include "VerifyParameterSetFixture.h"
#define WIN32_LEAN_AND_MEAN
#define STRICT
#include <windows.h>

std::string VerifyParameterSetFixture::equal()
{
	return false ? "true" : "false";
}
