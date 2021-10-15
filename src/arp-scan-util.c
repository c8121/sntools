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
 * Launches arp-scan, reads IP and MAC, resolves hostnames
 */

/*
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

#include "lib/mdb.c"
#include "lib/net_util.c"

char *arpScanCommand = "/usr/sbin/arp-scan --interface=%s --localnet -x";
char *interface = NULL;

int verbosity = 0;


/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:d:v";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'i':
			interface = optarg;
			printf("Select interface: %s\n", interface);
			break;

		case 'd':
			mdbDataDir = optarg;
			printf("Store data to: %s\n", mdbDataDir);
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

	if( prepareMdb() != 0 ) {
		exit(EX_IOERR);   
	}

	char cmdString[strlen(arpScanCommand)+32];
	sprintf(cmdString, arpScanCommand, interface);

	FILE *cmd = popen(cmdString, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit(EX_IOERR);
	}

	int cntNewItems = 0;
	int cntChangedItems = 0;

	char buf[254];
	while( fgets(buf, sizeof(buf), cmd) ) {

		if( verbosity > 0 ) {
			printf("arp-scan> %s", buf);
		}

		char *tok = strtok(buf, "\t");
		char ip[strlen(tok)+1];
		strcpy(ip, tok);

		tok = strtok(NULL, "\t");
		char mac[strlen(tok)+1];
		strcpy(mac, tok);

		char hostname[NI_MAXHOST];
		resolve_ip(ip, hostname, sizeof(hostname));

		if( verbosity > 0 ) {
			printf("IP: %s, MAC: %s, Name: %s\n", ip, mac, hostname);
		}

		// --- Check if IP existed before & if hostname changed
		MDB_val lastHostnameToIp;
		if( getMdb(ip, &lastHostnameToIp) == MDB_NOTFOUND ) {
			printf( "New IP: %s (Hostname=%s)\n", ip, hostname);
			cntNewItems++;
		} else if( strcmp(hostname, lastHostnameToIp.mv_data) != 0 ) {
			printf( "Hostname changed: old=%s, new=%s (IP=%s)\n", (char *)lastHostnameToIp.mv_data, hostname, ip);
			cntChangedItems++;
		}

		// --- Check if MAC existed before & if hostname changed
		MDB_val lastHostnameToMac;
		if( getMdb(mac, &lastHostnameToMac) == MDB_NOTFOUND ) {
			printf( "New MAC: %s (Hostname=%s)\n", mac, hostname);
			cntNewItems++;
		} else if( strcmp(hostname, lastHostnameToMac.mv_data) != 0 ) {
			printf( "Hostname changed: old=%s, new=%s (MAC=%s)\n", (char *)lastHostnameToMac.mv_data, hostname, mac);
			cntChangedItems++;
		}

		//Store ip -> hostname
		putMdb(ip, hostname);

		//Store mac -> hostname
		putMdb(mac, hostname);


		char key[64], ts[32];

		//Store mac -> ip
		sprintf(key, "MacIp:%s", mac);
		putMdb(key, ip);


		//Timestamp as char* fpr putMdb
		sprintf(ts, "%ld", (time_t)time(NULL));

		//Store ip -> timestamp
		sprintf(key, "%s_ts", ip);
		putMdb(key, ts);

		//Store mac -> timestamp
		sprintf(key, "%s_ts", mac);
		putMdb(key, ts);

	}

	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

	if( cntNewItems == 0 ) {
		printf( "No new items detected\n");
	} else {
		printf( "Notice: %i new items detected\n", cntNewItems);
	}

	if( cntChangedItems == 0 ) {
		printf( "No changes detected\n");
	} else {
		printf( "Notice: %i changes detected\n", cntChangedItems);
	}

	closeMdb();
}
