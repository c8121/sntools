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
#include <errno.h>

#define LISTEN_BACKLOG 10


/**
 * Create socket, bind & listen
 * @return socket file descriptor on success, -1 on fail
 */
int create_server_socket(char *ip, int port) {

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in bind_address;
	bind_address.sin_family = AF_INET;
	bind_address.sin_port = htons(port);
	bind_address.sin_addr.s_addr = inet_addr(ip);

	if( bind(server_socket, (struct sockaddr *)&bind_address, sizeof(bind_address)) != 0 ){
		fprintf(stderr, "Failed to bind to %s:%i\n", ip, port);
		return -1;
	}

	if( listen(server_socket, LISTEN_BACKLOG) != 0 ) {
		fprintf(stderr, "Cannot listen to %s:%i\n", ip, port);
		return -1;   
	}

	printf("Listening for connections at %s:%i\n", ip, port);

	return server_socket;
}


/**
 * Accept connections, call handle_request_function
 */
void accept_loop(int server_socket, void (*handle_request_function)(int client_socket, struct sockaddr_in *client_address)) {

	while(1) {

		struct sockaddr_in client_address;
		int c = sizeof(struct sockaddr_in);

		int client_socket = accept(server_socket, (struct sockaddr *)&client_address, (socklen_t*)&c);
		if( client_socket > -1 ) {

			char *client_ip = inet_ntoa(client_address.sin_addr);
			int client_port = ntohs(client_address.sin_port);
			printf("Accepted connection: %s:%i\n", client_ip, client_port);

			handle_request_function(client_socket, &client_address);
			close(client_socket);

		} else {
			fprintf(stderr, "accept failed\n");
		}
	}

}
