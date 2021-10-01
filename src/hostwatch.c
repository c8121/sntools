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

char *tcpdump_command = "tcpdump -ni %s";
char *interface = NULL;

int verbosity = 0;
int print_max_items = 20;
int ignore_direction = 0;
int print_interval_seconds = 3;
time_t last_print_data = 0;

struct host_data {
	char host_a[255];
	char host_b[255];
	unsigned int bytes;
};

struct linked_item *data = NULL;

/**
 * 
 */
struct linked_item* find_item(char *host_a, char *host_b) {
	struct linked_item *item = data;
	while (item != NULL) {
		struct host_data *data = item->data;
		if (strcmp(data->host_a, host_a) == 0) {
			if (strcmp(data->host_b, host_b) == 0) {	
				return item;
			}
		}
		item = item->next;
	}

	return NULL;
}

/**
 * 
 */
struct linked_item* create_item(char *host_a, char *host_b) {
	struct linked_item *item;
	if (data == NULL) {
		data = malloc(sizeof(struct linked_item));
		item = data;
	} else {
		struct linked_item *last = linked_item_last(data);
		item = malloc(sizeof(struct linked_item));
		last->next = item;
	}

	item->data = malloc(sizeof(struct host_data));
	struct host_data *data = (struct host_data*) item->data;
	strcpy(data->host_a, host_a);
	strcpy(data->host_b, host_b);
	data->bytes = 0;

	return item;
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:vx";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {

		case 'i':
			interface = optarg;
			printf("Select interface: %s\n", interface);
			break;

		case 'x':
			ignore_direction = 1;
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
void print_data() {

	if( verbosity == 0 ) {
		//Clear screen (POSIX only)
		printf("\e[1;1H\e[2J");
	}

	if (data == NULL) {
		printf("No data available yet...\n");
		return;
	}

	printf("---====A====---\t---====B====---\t---====bytes====---\n");

	struct linked_item *curr = data;
	struct host_data *curr_data;
	int num = 0;
	while (curr != NULL && num < print_max_items) {

		curr_data = (struct host_data*) curr->data;
		printf("%s\t%s\t%ub\n", curr_data->host_a, curr_data->host_b, curr_data->bytes);

		curr = curr->next;
		num++;
	}

	printf("\n");
	printf("Count: %i\n", linked_item_count(data));
}


/**
 * 
 */
void sort_data() {

	if (data == NULL) {
		return;
	}

	struct linked_item *last;
	struct host_data *last_data;
	struct linked_item *curr;
	struct host_data *curr_data;

	int changed = 0;
	do {
		changed = 0;
		last = NULL;
		curr = data;

		while( curr != NULL ) {

			if( last != NULL ) {
				last_data = (struct host_data*) last->data;
				curr_data = (struct host_data*) curr->data;
				if( curr_data->bytes > last_data->bytes ) {
					data = linked_item_remove(curr, data);
					data = linked_item_insert_before(curr, last, data);
					changed++;
				}
			}
			if( curr != NULL ) {
				last = curr;
				curr = curr->next;
			}
		}

	} while( changed > 0 );

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

	char cmdString[strlen(tcpdump_command) + 32];
	sprintf(cmdString, tcpdump_command, interface);

	FILE *cmd = popen(cmdString, "r");
	if (cmd == NULL) {
		fprintf(stderr, "Failed to execute: %s", cmdString);
		exit (EX_IOERR);
	}

	char line[1024];
	while (fgets(line, sizeof(line), cmd)) {

		if (verbosity > 2) {
			printf("tcpdump> %s", line);
		}

		//16:59:04.793423 IP 192.168.1.25.443 > 192.168.1.140.48246: Flags [P.], seq 241:544, ack 750, win 353, length 303
		//                  ^ p0
		//                                   ^ p1
		//                                                         ^ p2
		//                                                                                                      ^ p3

		char fromAddress[64];
		char toAddress[64];
		struct linked_item *item;
		struct host_data *item_data;

		char *p1 = strstr(line, " > ");
		if (p1 != NULL) {
			char *p2 = strstr(p1, ": ");
			if (p2 != NULL) {
				char *p0 = strstr(line, " ");
				if (p0 != NULL && (p0 = strstr(p0 + 1, " ")) != NULL
						&& p0 < p1) {

					char *p3 = strstr(p2, "length ");
					if (p3 != NULL) {

						memset(fromAddress, 0, sizeof(fromAddress));
						strncpy(fromAddress, p0 + 1, p1 - p0 - 1);

						memset(toAddress, 0, sizeof(toAddress));
						strncpy(toAddress, p1 + 3, p2 - p1 - 3);

						int bytes = atoi(p3 + 7);

						if (verbosity > 1) {
							printf("%s -> %s: %ib\n", fromAddress, toAddress,
									bytes);
						}

						item = find_item(fromAddress, toAddress);
						if (item == NULL) {
							if( ignore_direction ) {
								item = find_item(toAddress, fromAddress);
							}

							if (item == NULL) {
								item = create_item(fromAddress, toAddress);
							}
						}

						item_data = (struct host_data*) item->data;
						item_data->bytes += bytes;
						if (verbosity > 0) {
							printf("Update: %s -> %s: %ub\n", item_data->host_a,
									item_data->host_b, item_data->bytes);
						}

						sort_data();
						time_t curr_time = time(NULL);
						if( (curr_time - last_print_data) > (print_interval_seconds) ) {
							print_data();
							last_print_data = curr_time;
						}

					}

				}
			}
		}
	}

	if (feof(cmd)) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}