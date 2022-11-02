/*
 *  Copyright (c) 1999-2000 Vojtech Pavlik
 *  Copyright (c) 2009-2011 Red Hat, Inc
 *  Copyright (c) 2022 Miroslva Ondra, Faster CZ
 */

/**
 * @file
 * Event device test program
 *
 * evtest prints the capabilities on the kernel devices in /dev/input/eventX
 * and their events. Its primary purpose is for kernel or X driver
 * debugging.
 *
 * See INSTALL for installation details or manually compile with
 * gcc -o evtest evtest.c
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Should you need to contact me, the author, you can do so either by
 * e-mail - mail your message to <vojtech@ucw.cz>, or by paper mail:
 * Vojtech Pavlik, Simunkova 1594, Prague 8, 182 00 Czech Republic
 */

#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdint.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <linux/input.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/serial.h>

#ifndef input_event_sec
#define input_event_sec time.tv_sec
#define input_event_usec time.tv_usec
#endif

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)	((array[LONG(bit)] >> OFF(bit)) & 1)

#define DEV_INPUT_EVENT "/dev/input"
#define EVENT_DEV_NAME "event"

#ifndef EV_SYN
#define EV_SYN 0
#endif
#ifndef SYN_MAX
#define SYN_MAX 3
#define SYN_CNT (SYN_MAX + 1)
#endif
#ifndef SYN_MT_REPORT
#define SYN_MT_REPORT 2
#endif
#ifndef SYN_DROPPED
#define SYN_DROPPED 3
#endif

#define NAME_ELEMENT(element) [element] = #element

enum evtest_mode {
	MODE_CAPTURE,
	MODE_QUERY,
	MODE_VERSION,
};

static char* progname = "sw_485";
static volatile sig_atomic_t stop = 0;





static int version(void)
{
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "<version undefined>"
#endif
	printf("%s %s\n", progname, PACKAGE_VERSION);
	return EXIT_SUCCESS;
}


/**
 * Print usage information.
 */
static int usage(void)
{
	printf("USAGE:\n");
	printf(" Capture mode:\n");
	printf("   %s /dev/input/eventX /dev/ttymxc0\n", progname);
	printf("\n");
	printf(" Query mode: (check exit code)\n");
	printf("   %s --query /dev/input/eventX <value>\n", progname);

	printf("\n");
	printf("<value> numerical value 0-15 to monitor\n");

	return EXIT_FAILURE;
}

void get_tty_status(const char *tty)
{
	struct serial_rs485 rs485conf;
	int fd;

	/* Open your specific device (e.g., /dev/mydevice): */
	fd = open (tty, O_RDWR);
	if (fd < 0) {
		return;
	}
	memset(&rs485conf, 0, sizeof(rs485conf));
	if (!(ioctl (fd, TIOCGRS485, &rs485conf) < 0)) {
		printf("Port %s:\t%s mode\n", tty, (rs485conf.flags &SER_RS485_ENABLED)?"RS485":"RS232");
	}
	close (fd);
}

void set_tty_status(const char *tty, int use232)
{
	struct serial_rs485 rs485conf;
	int fd;

	/* Open your specific device (e.g., /dev/mydevice): */
	fd = open (tty, O_RDWR);
	if (fd < 0) {
		return;
	}
	memset(&rs485conf, 0, sizeof(rs485conf));
	if (!(ioctl (fd, TIOCGRS485, &rs485conf) < 0)) {
		//printf("ENA=%d, RTS=%d all=%x\n", rs485conf.flags &SER_RS485_ENABLED, !!(rs485conf.flags&SER_RS485_RTS_ON_SEND), rs485conf.flags);
	}
	if (use232) {
		if (rs485conf.flags & SER_RS485_ENABLED) {
			printf("Switching mode on %s to rs232\n", tty);
			rs485conf.flags &= ~(SER_RS485_ENABLED);
			ioctl (fd, TIOCSRS485, &rs485conf);
		}
	} else {
		if (!(rs485conf.flags & SER_RS485_ENABLED)) {
			printf("Switching mode on %s to rs485\n", tty);
			rs485conf.flags = SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND;
			ioctl (fd, TIOCSRS485, &rs485conf);
		}
	}
	close (fd);
}


static int get_switch_status(int fd, unsigned int code)
{
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	unsigned long state[KEY_CNT] = {0};

	// read supported event types
	memset(bit, 0, sizeof(bit));
	ioctl(fd, EVIOCGBIT(0, EV_MAX), bit[0]);
	if (!test_bit(EV_SW, bit[0])) return -1;    // unsupported type EW_SW
	// read supported sw codes
	ioctl(fd, EVIOCGBIT(EV_SW, KEY_MAX), bit[EV_SW]);
	if (!test_bit(code, bit[EV_SW])) return -1; // unsupported sw code
	// read switch state
	if (ioctl(fd, EVIOCGSW(sizeof(state)), state) >= 0) {
		return !!test_bit(code, state);
		//printf("%d\n", stateval);
		/*printf("    Event code %d (%s) state %d\n",
			       code, codename(EV_SW, code), stateval);*/
	}
	return 0;
}

/**
 * Print device events as they come in.
 *
 * @param fd The file descriptor to the device.
 * @return 0 on success or 1 otherwise.
 */
static int wait_events(int fd, unsigned int code, const char* tty)
{
	struct input_event ev[64];
	int i, rd;
	fd_set rdfs;

	FD_ZERO(&rdfs);
	FD_SET(fd, &rdfs);

	while (!stop) {
		select(fd + 1, &rdfs, NULL, NULL, NULL);
		if (stop)
			break;
		rd = read(fd, ev, sizeof(ev));

		if (rd < (int) sizeof(struct input_event)) {
			printf("expected %d bytes, got %d\n", (int) sizeof(struct input_event), rd);
			perror("\nevtest: error reading");
			return 1;
		}

		for (i = 0; i < rd / sizeof(struct input_event); i++) {
			//printf("Event: time %ld.%06ld, ", ev[i].input_event_sec, ev[i].input_event_usec);
			if ((ev[i].type == EV_SW) && (ev[i].code == code)) {
				set_tty_status(tty, ev[i].value);
				//printf("%d\n", ev[i].value);
			}
		}
	}

	return EXIT_SUCCESS;
}


/**
 * Enter capture mode. The requested event device will be monitored, and any
 * captured events will be decoded and printed on the console.
 *
 * @param device The device to monitor, or NULL if the user should be prompted.
 * @return 0 on success, non-zero on error.
 */
static int do_capture(const char *device, unsigned int code, const char* tty)
{
	int fd, rc;

	if ((fd = open(device, O_RDONLY)) < 0) {
		perror("evtest");
		if (errno == EACCES && getuid() != 0)
			fprintf(stderr, "You do not have access to %s. Try "
					"running as root instead.\n",
					device);
		return EXIT_FAILURE;
	}

	rc = get_switch_status(fd, code);
	if (rc < 0)
		return EXIT_FAILURE;
	set_tty_status(tty, rc);

	return wait_events(fd, code, tty);
}

/**
 * Perform a one-shot state query on a specific device. The query can be of
 * any known mode, on any valid keycode.
 *
 * @param device Path to the evdev device node that should be queried.
 * @param query_mode The event type that is being queried (e.g. key, switch)
 * @param keycode The code of the key/switch/sound/LED to be queried
 * @return 0 if the state bit is unset, 10 if the state bit is set, 1 on error.
 */
static int query_device(const char *device, int keycode, const char* tty)
{
	int fd;
	int r;
	unsigned long state[NBITS(SW_MAX)];

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return EXIT_FAILURE;
	}
	memset(state, 0, sizeof(state));
	r = ioctl(fd, EVIOCGSW(SW_MAX), state);
	close(fd);

	if (r == -1) {
		perror("ioctl");
		return EXIT_FAILURE;
	}
	if (tty) 
		get_tty_status(tty);

	if (test_bit(keycode, state)) {
		printf("Switch position:\tRS232 mode\n");
		return 10; /* different from EXIT_FAILURE */
	} else {
		printf("Switch position:\tRS485 mode\n");
		return 0;
	}
}

static const struct option long_options[] = {
	{ "query", no_argument, NULL, MODE_QUERY },
	{ "version", no_argument, NULL, MODE_VERSION },
	{ 0, },
};


int main (int argc, char **argv)
{
	const char *device = NULL;
	const char *tty = NULL;
//	const char *keyname = "15";
//	const char *event_type = "EV_SW";
	enum evtest_mode mode = MODE_CAPTURE;

	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 0:
			break;
		case MODE_QUERY:
			mode = c;
			break;
		case MODE_VERSION:
			return version();
		default:
			return usage();
		}
	}

	if (!(optind < argc)) {
		fprintf(stderr, "Requires event device as parameter\n");
		return usage();
	}
	device = argv[optind++];

	if (mode == MODE_QUERY) {
		if ((optind < argc))
			tty = argv[optind++];
		return query_device(device, 15, tty);
		//do_query(device, event_type, keyname);
	}

	if (!(optind < argc)) {
		fprintf(stderr, "Requires tty device as parameter\n");
		return usage();
	}
	tty = argv[optind++];

	return do_capture(device, 15, tty);
/*
	if ((argc - optind) > 1) {
		event_type = argv[optind++];
		keyname = argv[optind++];
	}
*/
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
