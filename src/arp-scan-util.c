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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

char *arpScanCommand = "/usr/sbin/arp-scan --interface=%s --localnet -x";
char *interface = NULL;

/**
* Resolve IP to Hostname
* @return 0 on success
*/
int resolveIpAddress(char *ip, char *resolvedName, int size) {

    struct sockaddr_in sa;
    char service[20];

    sa.sin_family = AF_INET;

    if( inet_pton(AF_INET, ip, &(sa.sin_addr)) != 1 ) {
        fprintf(stderr, "Failed to parse ip: %s\n", ip);
        return 1;
    }

    if( getnameinfo((struct sockaddr*)(&sa), sizeof(sa), resolvedName, size, service, sizeof(service), 0) !=0 ) {
        fprintf(stderr, "Name lookup failed: %s\n", ip);
        return 1;
    } else {
        return 0;
    }
}

/**
 *
 */
void configure(int argc, char *argv[]) {

    const char *options = "i:";
    int c;

    while ((c = getopt(argc, argv, options)) != -1) {
        switch(c) {
            case 'i':
                interface = optarg;
                printf("Select interface: %s\n", optarg);
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
        fprintf(stderr, "Please select an interdace (-i)\n");
        exit(EX_USAGE);
    }

    char *cmdString = malloc(strlen(arpScanCommand)+32);
    sprintf(cmdString, arpScanCommand, interface);

	FILE *cmd = popen(cmdString, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit(EX_IOERR);
	}
	
	char buf[254];
	while( fgets(buf, sizeof(buf), cmd) ) {
		
		printf("arp-scan> %s", buf);
		
		char *tok = strtok(buf, "\t");
		char ip[strlen(tok)+1];
		strcpy(ip, tok);
		
		tok = strtok(NULL, "\t");
		char mac[strlen(tok)+1];
		strcpy(mac, tok);

		char hostname[1024];
		resolveIpAddress(ip, hostname, sizeof(hostname));

		printf("IP: %s, MAC: %s, Name: %s\n", ip, mac, hostname);
	}
	
	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
