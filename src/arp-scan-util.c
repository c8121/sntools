#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>

char *arpScanCommand = "/usr/sbin/arp-scan --interface=wlo1 --localnet -x";


int main(int argc, char *argv[]) {

	printf("%s\n", arpScanCommand);
	
	FILE *cmd = popen(arpScanCommand, "r");
	if( cmd == NULL ) {
		fprintf(stderr, "Failed to execute: %s", arpScanCommand);
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
		fprintf(stderr, "Broken pipe: %s", arpScanCommand);
	}

}
