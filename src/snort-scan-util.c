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

#include "linked_items.c"

char *snort_command = "/usr/sbin/snort -d -i %s -h %s -A console -c %s";
char *interface = NULL;
char *home_network = NULL;
char *snort_conf = "/etc/snort/snort.conf";

int verbosity = 0;

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:h:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {

		case 'i':
			interface = optarg;
			printf("Interface: %s\n", interface);
			break;

		case 'h':
			home_network = optarg;
			printf("Home network: %s\n", home_network);
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
int main(int argc, char *argv[]) {

	configure(argc, argv);
	if (interface == NULL) {
		fprintf(stderr, "Please select an interface (-i)\n");
		exit (EX_USAGE);
	}
	if (home_network == NULL) {
		fprintf(stderr, "Please select a home network (-h)\n");
		exit (EX_USAGE);
	}

	char cmdString[strlen(snort_command) + 1024];
	sprintf(cmdString, snort_command, interface, home_network, snort_conf);

	FILE *cmd = popen(cmdString, "r");
	if (cmd == NULL) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit (EX_IOERR);
	}

	char line[1024];
	while (fgets(line, sizeof(line), cmd)) {

		if (verbosity > 0) {
			printf("snort> %s", line);
		}

		//CONSOLE mode output (-A console):
		//10/07-18:01:39.603639   [**] [1:1411:10] SNMP public access udp [**] [Classification: Attempted Information Leak] [Priority: 2] {UDP} 192.168.0.185:60860 -> 192.168.1.236:161
		//                                                                                                                                ^ p0
		//                                                                                                                                    ^ p1
		//                                                                                                                                                         ^ p2
		

		char *p0 = strstr(line, "{");
		char *p1 = strstr(line, "}");
		char *p2 = strstr(line, " -> ");
		if( p0 != NULL && p1 != NULL && p2 != NULL ) {
			
			char proto[10];
			memset(proto, 0, sizeof(proto));
			strncpy(proto, p0 + 1, p1 - p0 - 1);
			
			char host_src[128];
			memset(host_src, 0, sizeof(host_src));
			strncpy(host_src, p1 + 2, p2 - p1 - 2);
			
			char host_dst[128];
			strncpy(host_dst, p2 + 4, line + strlen(line) - p2 -2);
			
			printf("PROTO=%s, HOST_SRC=%s, HOST_DST=%s...\n", proto, host_src, host_dst);
		}
	}

	if (feof(cmd)) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
