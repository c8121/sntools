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

#include "../src/smtp_socket_util.c"
#include "../src/net_util.c"


/**
 * 
 */
int main(int argc, char *argv[]) {

	char *s = "Hello\n.\nTest\r\n.\r\nTest2=OK?\r\n012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\r\nEnd";

	char *smtpHost = "localhost";
	int smtpPort = 25;
	char *envFrom = "root";
	char *envTo = "root";
	char *heloName = "localhost";
	
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
		snprintf(buf, sizeof(buf), "Subject: Test\r\n");
		write(socket, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "From: <%s>\r\n", envFrom);
		write(socket, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "To: <%s>\r\n", envTo);
		write(socket, buf, strlen(buf));

		// Start body
		snprintf(buf, sizeof(buf), "\r\n");
		write(socket, buf, strlen(buf));

		
		smtp_qp_write(socket, s);
		

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

}
