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
 * This a...
 * - very simple HTTP server.
 * - very ineffective server (handles one client after another)
 * - very dangerous server (executes a given command and sends its output everytime a client connects)  
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
#include <errno.h>

#include "lib/net_srv_util.c"
#include "lib/net_util.c"
#include "lib/httpd_util.c"

#define LISTEN_BACKLOG 10

char *command;

int port = 8001;
char *ip = "0.0.0.0";
char *content_type = "text/plain";

int verbosity = 0;


/**
 * Handle HTTP-Request, send response (header, body)
 */
int respond(int client_socket) {

	char buf[1024];
	memset(buf, 0, sizeof(buf));

	if( init_response(client_socket, content_type, verbosity) < 0 ) {
		return -1;
	}


	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		writes(client_socket, "Failed to execute command");
	} else {

		int can_send = 1;
		while( fgets(buf, sizeof(buf), cmd) ) {
			if( can_send ) {
				if( writes(client_socket, buf) != 0 ) {
					can_send = 0;
					fprintf(stderr, "Lost connection during command: %s\n", command);
				}
			}
			if( !can_send ) {
				fprintf(stderr, "Cannot send: %s\n", buf);
			}
		}

		if( feof(cmd) ) {
			pclose(cmd);
		} else {
			fprintf(stderr, "Broken pipe: %s\n", command);
			return -1;
		}
	}

	return 0;
}

/**
 * 
 */
void handle_request(int client_socket, struct sockaddr_in *client_address) {
	respond(client_socket);
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "c:p:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'c':
			content_type = optarg;
			break;


		case 'p':
			port = atoi(optarg);
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
	printf("Usage: httpd-exec [-c content-type] [-p port] \"<command>\"\n");
}


/**
 * Start server, wait for connections and execute command once for each client
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if (argc - optind +1 < 2) {
		fprintf(stderr, "Missing arguments\n");
		usage_message();
		exit(EX_USAGE);
	}

	//Command
	command = argv[optind];
	if( command[0] == '\0' ) {
		fprintf(stderr, "No command given\n");
		usage_message();
		exit(EX_USAGE);
	}

	printf("WARNING: The command '%s' will be executed every time a client connects and the output will sent directly to the client!\n", command);
	printf("WARNING: USE AT OWN RISK!\n");

	int server_socket = create_server_socket(ip, port);
	if( server_socket < 0 ) {
		exit(EX_IOERR);
	}

	accept_loop(server_socket, &handle_request);
}
