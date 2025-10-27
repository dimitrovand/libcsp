#pragma once

#include <csp/autoconfig.h> // compile configuration

#if (CSP_USE_CUSTOM_ENDIAN_H)
#include "csp_custom_endian.h"
#else
#include <endian.h>
#endif
