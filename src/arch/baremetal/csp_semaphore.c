#include "csp_semaphore.h"
#include "csp_macro.h"

__weak void csp_bin_sem_init(csp_bin_sem_t * sem) {
	return;
}

__weak int csp_bin_sem_wait(csp_bin_sem_t * sem, unsigned int timeout) {
	return CSP_SEMAPHORE_OK;
}

__weak int csp_bin_sem_post(csp_bin_sem_t * sem) {
	return CSP_SEMAPHORE_OK;
}
