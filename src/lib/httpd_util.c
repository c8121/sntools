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


/**
 * Read request headers (ignore them), send response header (HTTP1.1, Connection-Close)
 */
int init_response(int client_socket, char *content_type, int verbosity) {

	char buf[1024];
	memset(buf, 0, sizeof(buf));

	//Read request headers
	ssize_t r;
	while( (r = read(client_socket, buf, sizeof(buf))) > 0 ) {
		if( verbosity > 0 ) {
			printf("%s\n", buf);
		}
		if( strstr(buf, "\r\n\r\n") != NULL || strstr(buf, "\n\n") != NULL )
			break;
		memset(buf, 0, sizeof(buf));
	}

	if( r < 0 ) {
		fprintf(stderr, "### Failed to read request\n");
		fprintf(stderr, "### %s\n", strerror(errno));
		return -1;
	}

	if( writes(client_socket, "HTTP/1.1 200 OK\r\n") != 0 )
		return -1;

	sprintf(buf, "Content-Type: %s\r\n", content_type);
	if( writes(client_socket, buf) != 0 )
		return -1;

	if( writes(client_socket, "Connection: close\r\n\r\n") != 0 )
		return -1;
	
	return 0;
}
