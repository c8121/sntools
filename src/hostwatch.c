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

char *tcpDumpCommand = "tcpdump -ni %s";
char *interface = NULL;

int verbosity = 0;

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'i':
			interface = optarg;
			printf("Select interface: %s\n", interface);
			break;

		case 'v':
			verbosity = 1;
			break;
		}
	}
}

/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);
	if( interface == NULL ) {
		fprintf(stderr, "Please select an interface (-i)\n");
		exit(EX_USAGE);
	}

	char cmdString[strlen(tcpDumpCommand)+32];
	sprintf(cmdString, tcpDumpCommand, interface);

	FILE *cmd = popen(cmdString, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit(EX_IOERR);
	}

	char line[1024];
	while( fgets(line, sizeof(line), cmd) ) {

		if( verbosity > 0 ) {
			printf("tcpdump> %s", line);
		}

		//16:59:04.793423 IP 192.168.1.25.443 > 192.168.1.140.48246: Flags [P.], seq 241:544, ack 750, win 353, length 303
		//                  ^ p0
		//                                   ^ p1
		//                                                         ^ p2
		//                                                                                                      ^ p3


		char *p1 = strstr(line, " > ");
		if( p1 != NULL ) {
			char *p2 = strstr(p1, ": ");
			if( p2 != NULL ) {
				char *p0 = strstr(line, " ");
				if( p0 != NULL && (p0 = strstr(p0+1, " ")) != NULL && p0 < p1 ) {

					char *p3 = strstr(p2, "length ");
					if( p3 != NULL ) {
					
						char fromAddress[64];
						memset(fromAddress, 0, sizeof(fromAddress));
						strncpy(fromAddress, p0 +1, p1 -p0 -1);
	
						char toAddress[64];
						memset(toAddress, 0, sizeof(toAddress));
						strncpy(toAddress, p1 +3, p2 -p1 -3);
	
						int bytes = atoi(p3 +7);
						
						printf("%s -> %s: %ib\n", fromAddress, toAddress, bytes);
						
					}

				}
			}
		}
	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
