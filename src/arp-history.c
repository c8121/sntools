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
char outputFormat = '0';


/**
 * Read command line arguments and configure application
 * Create data directoy
 */
void configure(int argc, char *argv[]) {


	// -- Read CLI arguments -------

	const char *options = "q:m:d:t";
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

		case 't':
			outputFormat = 't';
			printf("Load data from: %s\n", mdbDataDir);
			break;
		}
	}
}

/**
 * 
 */
void printAllData() {

	MDB_cursor *cursor;
	if( mdb_cursor_open(mdbTxn, mdbDbi, &cursor) != MDB_SUCCESS ) {
		fprintf(stderr, "Failed to open cursor\n");
	} else {
		MDB_val key, value;
		while( mdb_cursor_get(cursor, &key, &value, MDB_NEXT) == MDB_SUCCESS ) {
			printf("%s = %s\n", (char *)key.mv_data, (char *)value.mv_data);
		}
		mdb_cursor_close(cursor);
	}

}

/**
 * 
 */
void printTable() {

	MDB_cursor *cursor;
	if( mdb_cursor_open(mdbTxn, mdbDbi, &cursor) != MDB_SUCCESS ) {
		fprintf(stderr, "Failed to open cursor\n");
	} else {
		MDB_val key, value;
		key.mv_data = "MacIp:";
		key.mv_size = 6;
		if( mdb_cursor_get(cursor, &key, &value, MDB_SET_RANGE) == MDB_SUCCESS ) {

			printf("MAC              \tIP              TS         \tDATE\n");

			while( mdb_cursor_get(cursor, &key, &value, MDB_NEXT) == MDB_SUCCESS ) {

				char *keyName = key.mv_data;
				if( strlen(keyName) < 6 || strncmp(keyName, "MacIp:", 6) != 0 ) {
					break;
				}

				char *mac = &keyName[6];
				char *ip = (char *)value.mv_data;

				char tsKey[64];
				sprintf(tsKey, "%s_ts", mac);

				char *ts;
				MDB_val tsData;
				if( getMdb(tsKey, &tsData) == MDB_NOTFOUND ) {
					ts = '\0';
				} else {
					ts = (char*)tsData.mv_data;
				}

				time_t tsl = strtol(ts, NULL, 0);
				struct tm *lt = localtime(&tsl);
				char timeString[32];
				strftime(timeString, sizeof(timeString), "%a %b %d %Y", lt);

				printf("%s\t%s\t%s\t%s\n", mac, ip, ts, timeString);
			}
		}
		mdb_cursor_close(cursor);
	}

}


/**
 * Launches arp-scan, reads IP and MAC, resolves hostnames
 * CLI Arguments:
 *   -q <ip or mac> (optional)
 *   -d <mdb directory> (optional)
 *   -t (generate table, optional)
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
	} else if ( outputFormat == 't' ) {

		printTable();

	} else {

		printAllData();

	}

	closeMdb();
}
