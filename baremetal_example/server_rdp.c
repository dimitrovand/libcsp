#include <csp/csp.h>
#include <csp/csp_buffer.h>
#include <csp/interfaces/csp_if_lo.h>
#include <csp/interfaces/csp_if_kiss.h>
#include <csp/drivers/usart.h>
#include <csp/csp_debug.h>
#include <csp/arch/csp_time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#define SERVER_PORT 10
#define SERVER_ADDR 1
#define DEVICE_NAME "/tmp/pty1"
#define BAUDRATE    115200

/** Needed by CSP */
void csp_usart_lock(void * driver_data) {
	return;
}

/** Needed by CSP */
void csp_usart_unlock(void * driver_data) {
	return;
}

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

static int uart_init(const char * device, speed_t brate) {
	int fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		csp_print("%s: failed to open device: [%s], errno: %s\n", __func__, device, strerror(errno));
		exit(1);
	}

	struct termios options;
	tcgetattr(fd, &options);
	cfsetispeed(&options, brate);
	cfsetospeed(&options, brate);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	options.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	options.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | INPCK | ISTRIP | IXON);
	options.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | OFILL | OPOST);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1;

	/* tcsetattr() succeeds if just one attribute was changed, should read back attributes and check all has been changed */
	if (tcsetattr(fd, TCSANOW, &options) != 0) {
		csp_print("%s: Failed to set attributes on device: [%s], errno: %s\n", __func__, device, strerror(errno));
		close(fd);
		exit(1);
	}

	/* Flush old transmissions */
	if (tcflush(fd, TCIOFLUSH) != 0) {
		csp_print("%s: Error flushing device: [%s], errno: %s\n", __func__, device, strerror(errno));
		close(fd);
		exit(1);
	}

	return fd;
}

static uint8_t cbuf[400];

static void uart_rx(int fd, csp_iface_t * iface) {
    int length = read(fd, cbuf, sizeof(cbuf));
    if (length > 0) {
        csp_kiss_rx(iface, cbuf, length, NULL);
    }
}

static int uart_tx(void * driver_data, const uint8_t * data, size_t len) {
    int fd = *((int*)driver_data);
	int res = write(fd, data, len);
	if (res == len) {
		return CSP_ERR_NONE;
	}

	return CSP_ERR_TX; 
}

int main() {
    csp_init();

    int fd = uart_init(DEVICE_NAME, BAUDRATE);

    csp_iface_t kiss_iface;
    csp_kiss_interface_data_t kiss_ifdata;
    kiss_ifdata.tx_func = uart_tx;
    kiss_iface.interface_data = &kiss_ifdata;
    kiss_iface.name = CSP_IF_KISS_DEFAULT_NAME;
    kiss_iface.driver_data = &fd;
    kiss_iface.addr = SERVER_ADDR;

    if (CSP_ERR_NONE != csp_kiss_add_interface(&kiss_iface)) {
        exit(1);
    }

    kiss_iface.is_default = 1;

	csp_socket_t sock = {0};
	sock.opts = CSP_SO_CRC32REQ | CSP_SO_RDPREQ;
	csp_bind(&sock, CSP_ANY);
	csp_listen(&sock, 0);

	while (1) {
        /* these must be called periodically */
        csp_route_work();
        uart_rx(fd, &kiss_iface);

		/* Poll for a new connection */
		csp_conn_t * conn;
		if ((conn = csp_accept(&sock, 0)) != NULL) {
            /* Poll for packets on connection */
            csp_packet_t * packet;
            uint8_t retry = 0;
            while (retry < 100) {
                if ((packet = csp_read(conn, 0)) != NULL) {
                    /* send back whatever we've received */
					csp_send(conn, packet);
					break;
                }

                retry++;
                usleep(10000);

                /* these must be called periodically */
                csp_route_work();
                uart_rx(fd, &kiss_iface);
            }

            csp_close(conn);
        }
	}
}
