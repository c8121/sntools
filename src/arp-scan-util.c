#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <unistd.h>

char *arpScanCommand = "/usr/sbin/arp-scan --interface=%s --localnet -x";
char *interface = NULL;

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
		
		printf("> %s", buf);
		
		char *tok = strtok(buf, "\t");
		char ip[strlen(tok)+1];
		strcpy(ip, tok);
		
		tok = strtok(buf, "\t");
		char mac[strlen(tok)+1];
		strcpy(mac, tok);
		
		printf("IP: %s, MAC: %s\n", ip, mac);
	}
	
	if( feof(cmd) ) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
