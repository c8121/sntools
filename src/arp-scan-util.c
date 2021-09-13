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

#include "lmdb.h"

char *arpScanCommand = "/usr/sbin/arp-scan --interface=%s --localnet -x";
char *interface = NULL;

char *mdbDataDir = "/var/spool/sntools";
mode_t mdbDataMode = 0664;

MDB_env *mdbEnv;
MDB_txn *mdbTxn;
MDB_dbi mdbDbi;

/**
* Resolve IP to Hostname
* @return 0 on success
*/
int resolveIpAddress(char *ip, char *resolvedName, int size) {

    struct sockaddr_in sa;
    char service[NI_MAXSERV];

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
* Create LMDB resources
* @return 0 on success
*/
int prepareMdb() {
	
    if( mdb_env_create(&mdbEnv) != MDB_SUCCESS ) {
        fprintf(stderr, "Failed to create LMDB environment");
        return 1;
    }
    
    if( mdb_env_open(mdbEnv, mdbDataDir, 0, mdbDataMode) != MDB_SUCCESS ) {
        mdb_env_close(mdbEnv);
        fprintf(stderr, "Failed to open LMDB environment");
        return 1;
    }
    
    if( mdb_txn_begin(mdbEnv, NULL, 0, &mdbTxn) != MDB_SUCCESS ) {
        mdb_env_close(mdbEnv);
        fprintf(stderr, "Failed to begin LMDB transaction");
        return 1;
    }
    
    if( mdb_dbi_open(mdbTxn, NULL, 0, &mdbDbi) != MDB_SUCCESS ) {
        mdb_txn_abort(mdbTxn);
        mdb_env_close(mdbEnv);
        fprintf(stderr, "Failed to open LMDB database");
        return 1;
    }
    
	return 0;

}

/**
*
*/
void closeMdb() {

    if( mdb_txn_commit(mdbTxn) != MDB_SUCCESS ) {
        fprintf(stderr, "Failed to commit LMDB transaction");
    }
    
    mdb_dbi_close(mdbEnv, mdbDbi);
    mdb_env_close(mdbEnv);

}

/**
*
*/
int getMdb(char *key, MDB_val *value) {

    MDB_val key_val;
    key_val.mv_size = strlen(key)+1;
    key_val.mv_data = key;
    
    return mdb_get(mdbTxn, mdbDbi, &key_val, value);
}

/**
*
*/
void putMdb(char *key, char *value) {

    MDB_val key_val;
    key_val.mv_size = strlen(key)+1;
    key_val.mv_data = key;
    
    MDB_val data_val;
    data_val.mv_size = strlen(value)+1;
    data_val.mv_data = value;
    
    if( mdb_put(mdbTxn, mdbDbi, &key_val, &data_val, 0) != MDB_SUCCESS ) {
        fprintf(stderr, "Failed to store key %s", key);
    }
}


/**
 * Read command line arguments and configure application
 * Create data directoy
 */
void configure(int argc, char *argv[]) {


    // -- Read CLI arguments -------
    
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
    
    // -- Check/create data dir -----
    
    struct stat fileStat;
    if( stat(mdbDataDir, &fileStat) != 0 ) {
        printf("Create data directory: %s", mdbDataDir);
        if( mkdir(mdbDataDir, mdbDataMode) != 0 ) {
            fprintf(stderr, "Failed to create %s", mdbDataDir);
        }
    }
        

}

/**
* Launches arp-scan, reads IP and MAC, resolves hostnames
* CLI Arguments:
*   -i <interface name>
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

    char *cmdString = malloc(strlen(arpScanCommand)+32);
    sprintf(cmdString, arpScanCommand, interface);

	FILE *cmd = popen(cmdString, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit(EX_IOERR);
	}
	
	char buf[254];
	while( fgets(buf, sizeof(buf), cmd) ) {
		
		//printf("arp-scan> %s", buf);
		
		char *tok = strtok(buf, "\t");
		char ip[strlen(tok)+1];
		strcpy(ip, tok);
		
		tok = strtok(NULL, "\t");
		char mac[strlen(tok)+1];
		strcpy(mac, tok);

		char hostname[NI_MAXHOST];
		resolveIpAddress(ip, hostname, sizeof(hostname));

		//printf("IP: %s, MAC: %s, Name: %s\n", ip, mac, hostname);
		
		// --- Check if IP existed before & if hostname changed
		MDB_val lastHostnameToIp;
		if( getMdb(ip, &lastHostnameToIp) == MDB_NOTFOUND ) {
		    printf( "New IP: %s (Hostname=%s)\n", ip, hostname);
		} else if( strcmp(hostname, lastHostnameToIp.mv_data) != 0 ) {
		    printf( "Hostname changed: old=%s, new=%s (IP=%s)\n", (char *)lastHostnameToIp.mv_data, hostname, ip);
		}
		
		// --- Check if MAC existed before & if hostname changed
		MDB_val lastHostnameToMac;
		if( getMdb(mac, &lastHostnameToMac) == MDB_NOTFOUND ) {
		    printf( "New MAC: %s (Hostname=%s)\n", mac, hostname);
		} else if( strcmp(hostname, lastHostnameToMac.mv_data) != 0 ) {
		    printf( "Hostname changed: old=%s, new=%s (MAC=%s)\n", (char *)lastHostnameToMac.mv_data, hostname, mac);
		}
		
		putMdb(ip, hostname);
		putMdb(mac, hostname);
		
	}
	
	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

    closeMdb();
}
