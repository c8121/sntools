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

#define LISTEN_BACKLOG 10

char *command;

int port = 8001;
char *ip = "0.0.0.0";
char *content_type = "text/plain";

void writes(int client_socket, char *line) {
	if( send(client_socket, line, strlen(line), 0) < 0 ) {
		fprintf(stderr, "Failed to send response\n");
	}
}

/**
 */
void respond(int client_socket) {

	char buf[1024];
	memset(buf, 0, sizeof(buf));

	//Read request headers
	while( read(client_socket, buf, sizeof(buf)) > 0 ) {
		printf("%s\n", buf);
		if( strstr(buf, "\r\n\r\n") != NULL || strstr(buf, "\n\n") != NULL )
			break;
		memset(buf, 0, sizeof(buf));
	}

	writes(client_socket, "HTTP/1.1 200 OK\r\n");

	sprintf(buf, "Content-Type: %s\r\n", content_type);
	writes(client_socket, buf);

	writes(client_socket, "Connection: close\r\n");
	writes(client_socket, "\r\n");


	FILE *cmd = popen(command, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s\n", command);
		writes(client_socket, "Failed to execute command");
	} else {

		while( fgets(buf, sizeof(buf), cmd) ) {
			writes(client_socket, buf);
		}

		if( feof(cmd) ) {
			pclose(cmd);
		} else {
			fprintf(stderr, "Broken pipe: %s", command);
		}
	}

}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "c:p:";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'c':
			content_type = optarg;
			break;


		case 'p':
			port = atoi(optarg);
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

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in bind_address;
	bind_address.sin_family = AF_INET;
	bind_address.sin_port = htons(port);
	bind_address.sin_addr.s_addr = inet_addr(ip);

	if( bind(server_socket, (struct sockaddr *)&bind_address, sizeof(bind_address)) != 0 ){
		fprintf(stderr, "Failed to bind to %s:%i\n", ip, port);
		exit(EX_IOERR);
	}

	if( listen(server_socket, LISTEN_BACKLOG) != 0 ) {
		fprintf(stderr, "Cannot listen to %s:%i\n", ip, port);
		exit(EX_IOERR);   
	}

	printf("Listening for connections at %s:%i\n", ip, port);

	while(1) {
		int client_socket = accept(server_socket, NULL, NULL);
		if( client_socket > -1 ) {
			printf("Accepted connection\n");
			respond(client_socket);
			close(client_socket);
		} else {
			fprintf(stderr, "accept failed\n");
		}
	}
}
