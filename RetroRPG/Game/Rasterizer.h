#pragma once

#include "Core.h"

#if RENDERER_INTRASTER
#	include "RasterizerInteger.h"
#else
#	include "RasterizerFloat.h"
#endif