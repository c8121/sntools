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
#include <time.h>

#include "mdb.c"


char *query = NULL;


/**
 * Read command line arguments and configure application
 * Create data directoy
 */
void configure(int argc, char *argv[]) {


	// -- Read CLI arguments -------

	const char *options = "q:m:d:";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		case 'q':
			query = optarg;
			printf("Query: %s\n", query);
			break;

		case 'd':
			mdbDataDir = optarg;
			printf("Load data from: %s\n", mdbDataDir);
			break;
		}
	}
}

/**
 * Launches arp-scan, reads IP and MAC, resolves hostnames
 * CLI Arguments:
 *   -q <ip or mac> (optional)
 *   -d <mdb directory> (optional)
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);

	if( prepareMdb() != 0 ) {
		exit(EX_IOERR);   
	}


	if( query != NULL ) {

		char tsKey[64];
		sprintf(tsKey, "%s_ts", query);

		MDB_val tsData;
		if( getMdb(tsKey, &tsData) == MDB_NOTFOUND ) {
			printf( "'%s' not found\n", query);
		} else {
			printf( "Last seen at: %s\n", (char *)tsData.mv_data);
		}

	}

	closeMdb();
}
