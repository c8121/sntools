/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Author: christian c8121 de
 */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "net_util.c"
#include "smtp_socket_util.c"
#include "linked_items.c"

char *command;
char *smtpHost;
int smtpPort;
char *envFrom;
char *envTo;
char *heloName;
char *subject = "Output from command";

int waitTimeout = 0;

int verbosity = 0;


pthread_mutex_t handle_data_mutex;

struct linked_item *buffer = NULL;
int bufferMaxLines = 100;


/**
 *
 */
int send_buffer() {

	int socket = open_socket(smtpHost, smtpPort);
	char buf[1024];
	if( socket > 0 ) {

		//Read SMTP Greeting
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "helo %s\r\n", heloName);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "mail from: <%s>\r\n", envFrom);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "rcpt to: <%s>\r\n", envTo);
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "data\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		// Send header
		snprintf(buf, sizeof(buf), "Subject: %s\r\n", subject);
		write(socket, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "From: <%s>\r\n", envFrom);
		write(socket, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "To: <%s>\r\n", envTo);
		write(socket, buf, strlen(buf));

		// Start body
		snprintf(buf, sizeof(buf), "\r\n");
		write(socket, buf, strlen(buf));

		// Send body which has been read above
		struct linked_item *curr = buffer;
		while( curr != NULL ) {
			smtp_write(socket, curr->data);
			curr = curr->next;
		}

		snprintf(buf, sizeof(buf), "\r\n.\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		snprintf(buf, sizeof(buf), "quit\r\n");
		write(socket, buf, strlen(buf));
		if( smtp_read(socket) != 0 ) {
			return -1;
		}

		close(socket);
	}

	return 0;
}


/**
 * Check if data was read from command.
 * Send if enough data available (or if force is not 0)
 */
void handle_data(int force) {

	if( verbosity > 1 ) {
		printf("check if data should be sent (force=%i)\n", force);
	}

	pthread_mutex_lock(&handle_data_mutex);
	if( buffer == NULL ) {
		pthread_mutex_unlock(&handle_data_mutex);
		return;
	}

	if( force || linked_item_count(buffer) >= bufferMaxLines ) {

		if( verbosity > 1 ) {
			struct linked_item *out = buffer;
			while( out != NULL ) {
				printf("SEND> %s", (char*)out->data);
				out = out->next;
			}
		}

		send_buffer();

		linked_item_free(buffer);
		buffer = NULL;
	}

	pthread_mutex_unlock(&handle_data_mutex);

}


/**
 *
 */
void *timer() {
	while( 1 ) {
		sleep(waitTimeout);
		handle_data(1);
	}
}

/**
 *
 */
void create_timer() {
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, timer, NULL);
}


/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "s:c:t:vv";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 's':
			subject = optarg;
			break;

		case 't':
			waitTimeout = atoi(optarg);
			printf("Wait timeout: %i seconds\n", waitTimeout);
			break;

		case 'c':
			bufferMaxLines = atoi(optarg);
			printf("Set buffer size: %i lines\n", bufferMaxLines);
			break;

		case 'v':
			verbosity++;
			break;
		}
	}
}

/**
 * 
 */
void usage_message() {
	printf("Usage: exec-and-mail [-c buffer-line-count] [-t wait-timeout-seconds] [-v] [-s subject] <host> <port> <from> <to> \"<command>\"\n");
}


/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if (argc - optind +1 < 6) {
		fprintf(stderr, "Missing arguments\n");
		usage_message();
		exit(EX_USAGE);
	}

	// SMTP Server
	smtpHost = argv[optind];
	smtpPort = atoi(argv[optind+1]);

	// Check sender and recipient
	envFrom = argv[optind+2];
	envTo = argv[optind+3];
	if( envFrom[0] == '\0' || envTo[0] == '\0' ) {
		fprintf(stderr, "Parameter 'from' and 'to' cannot be empty\n");
		usage_message();
		exit(EX_USAGE);
	}

	// Read hostname
	char buf[128];
	gethostname(buf, sizeof(buf));
	heloName = buf;

	//Command
	command = argv[optind+4];
	if( command[0] == '\0' ) {
		fprintf(stderr, "No command given\n");
		usage_message();
		exit(EX_USAGE);
	}

	if( waitTimeout > 0 ) {
		create_timer();
	}

	printf("Execute: %s\n", command);

	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", command);
		exit(EX_IOERR);
	}

	struct linked_item *readInto = NULL;

	char line[1024];
	while( fgets(line, sizeof(line), cmd) ) {

		if( verbosity > 0 ) {
			printf("command> %s", line);
		}

		pthread_mutex_lock(&handle_data_mutex);

		if( buffer == NULL ) {
			buffer = malloc(sizeof(struct linked_item));
			readInto = buffer;
		} else {
			readInto->next = malloc(sizeof(struct linked_item));
			readInto = readInto->next;
		}

		readInto->next = NULL;

		int len = strlen(line);
		readInto->data = malloc(len+1);
		strcpy(readInto->data, line);

		pthread_mutex_unlock(&handle_data_mutex);

		handle_data(0);
	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", command);
	}

	handle_data(1);
}
