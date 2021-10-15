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

/*
 * Author: christian c8121 de
 */


/**
 * Read and check server response.
 * Return -1 on error response, 0 otherwise
 */
int smtp_read(int socket) {

	char response[255];
	char c;
	int i=0;
	while( 1 ) {

		int n = read(socket, &c, 1);
		if( n < 1 ) {
			return -1;
		}

		if( c == '\n' ) {

			if( i > 0 && (response[0] == '2' || response[0] == '3') ) {
				return 0;
			} else {
				fprintf(stderr, "Got error response: %s\n", response);
				return -1;
			}
		}

		if( i < 254 ) {
			response[i] = c;
			response[i+1] ='\0';
			i++;
		}
	}

}

/**
 * Write data to socket.
 * Checks if data starts with .\r\n (or .\n) and sends ..\r\n
 */
void smtp_write(int socket, char *data) {

	if (strlen(data) > 1 && data[0] == '.' && (data[1] == '\r' || data[1] == '\n')) {
		write(socket, ".", 1);
	}
	write(socket, data, strlen(data));
}

/**
 * Write text in quoted-printable encoding to socket.
 */
void smtp_qp_write(int socket, char *data) {

	int n = strlen(data);
	int li = 0;
	for( int i=0 ; i<n ; i++ ) {
		if( li == 0 && data[i] == '.' && i < (n-2) && data[i+1] == '\r' && data[i+2] == '\n' ) {
			write(socket, ".", 1);
			li++;
		}
		
		if( data[i] == '\n' ) {
			//Single <LF> to <CR><LF>
			write(socket, "\r\n", 2);
		} else if( i < (n-1) && data[i] == '\r' && data[i+1] == '\n' ) {
			//<CR><LF> as it is
			write(socket, "\r\n", 2);
			i++;
		} else if ( data[i] == '\r' ) {
			//Single <CR> to <CR><LF>
			write(socket, "\r\n", 2);
			i++;
		} else if( (data[i] >= 32 && data[i] <= 60) || (data[i] >= 62 && data[i] <= 126) ) {
			//Allowed chars
			write(socket, (char*)&data[i], 1);
			li++;
		} else {
			//Encoded chars
			char tmp[4];
			sprintf(tmp, "=%02X", data[i]);
			write(socket, tmp, 3);
			li += 3;
		}

		if( data[i] == '\r' || data[i] == '\n' ) {
			//New line just started, reset line counter
			li = 0;
		} else if( li == 75 ) {
			//Max line length reached
			write(socket, "=\r\n", 3);
			li = 0;
		}
	}

}



