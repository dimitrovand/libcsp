#include <csp/arch/csp_time.h>
#include "csp_macro.h"

__weak uint32_t csp_get_ms(void) {
	return 0;
}

__weak uint32_t csp_get_ms_isr(void) {
	return 0;
}

__weak uint32_t csp_get_s(void) {
	return 0;
}

__weak uint32_t csp_get_s_isr(void) {
	return 0;
}
