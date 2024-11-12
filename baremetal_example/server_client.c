#include <csp/csp.h>
#include <csp/csp_buffer.h>
#include <csp/interfaces/csp_if_lo.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#define SERVER_PORT 10
#define SERVER_ADDR 0
#define DEVICE_NAME "/tmp/pty1"
#define BAUDRATE    115200

/** Needed by CSP */
uint32_t csp_get_ms(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
	}
	return 0;
}

/** Needed by CSP */
uint32_t csp_get_ms_isr(void) {
	return csp_get_ms();
}

/** Needed by CSP */
uint32_t csp_get_s(void) {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		return (uint32_t)ts.tv_sec;
	}
	return 0;
}

/** Needed by CSP */
uint32_t csp_get_s_isr(void) {
	return csp_get_s();
}

int main() {
    csp_init();

    // for the sake of simplicity no interfaces are 
    // configured - run server and client using loopback interface.
    csp_if_lo.is_default = 1;

	csp_socket_t sock = {0};
    sock.opts |= CSP_SO_CRC32REQ;
    const uint8_t server_port = 1;
    csp_bind(&sock, server_port);
	csp_listen(&sock, 0);
    const uint8_t client_port = 2; 
    uint8_t count = 0;

    while (1) {
        // must be called periodically
        csp_route_work();

        // this is the client part
        csp_conn_t * cc = csp_connect(CSP_PRIO_NORM, SERVER_ADDR, server_port, 0, CSP_O_CRC32);
        if (cc) {
            csp_packet_t * pc = csp_buffer_get(0);
            if (pc) {
                pc->data[0] = count++;
                pc->length = 1;
                csp_send(cc, pc);
            }

            csp_close(cc);
        }

        // and this is the server part
        csp_conn_t * cs = csp_accept(&sock, 0);
        if (cs) { 
            csp_packet_t * ps = csp_read(cs, 0);
            if (ps) {
                printf("received %d\n", ps->data[0]);
                csp_buffer_free(ps);
            }

            csp_close(cs);
        }
    }
}
