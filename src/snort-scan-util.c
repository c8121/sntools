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

int print_max_items = 20;
int strip_src_port = 0;
int verbosity = 0;

struct event_data {
	char host_src[255];
	char host_dst[255];
	int hit_count;
	char latest_proto[10];
	char latest_message[1024];
	int latest_prio;
};

struct linked_item *data = NULL;

/**
 * 
 */
struct linked_item* find_item(char *host_src, char *host_dst) {
	struct linked_item *item = data;
	while (item != NULL) {
		struct event_data *data = item->data;
		if (strcmp(data->host_src, host_src) == 0) {
			if (strcmp(data->host_dst, host_dst) == 0) {	
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
struct linked_item* create_item(char *host_src, char *host_dst) {
	struct linked_item *item;
	if (data == NULL) {
		data = malloc(sizeof(struct linked_item));
		item = data;
	} else {
		struct linked_item *last = linked_item_last(data);
		item = malloc(sizeof(struct linked_item));
		last->next = item;
	}

	item->data = malloc(sizeof(struct event_data));
	struct event_data *data = (struct event_data*) item->data;
	strcpy(data->host_src, host_src);
	strcpy(data->host_dst, host_dst);
	data->hit_count = 0;
	data->latest_proto[0] = '\0';
	data->latest_message[0] = '\0';
	data->latest_prio = 99;

	return item;
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:h:vm";
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

		case 'm':
			strip_src_port = 1;
			printf("Will strip port from source\n");
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


	struct linked_item *curr = data;
	struct event_data *curr_data;
	int num = 0;
	while (curr != NULL && num < print_max_items) {

		curr_data = (struct event_data*) curr->data;
		printf("%s\t%s\t(%s)\n", curr_data->host_src, curr_data->host_dst, curr_data->latest_proto);
		printf("\tHits: %i\n", curr_data->hit_count);
		printf("\t%s\n", curr_data->latest_message);

		curr = curr->next;
		num++;
	}

	printf("\n");
	printf("Count: %i\n", linked_item_count(data));
}

/**
 * comparison of prio and hits, used by linked_item_sort
 */
int compare(struct linked_item *a, struct linked_item *b) {
	struct event_data *a_data = (struct event_data*) a->data;
	struct event_data *b_data = (struct event_data*) b->data;
	unsigned long a_score = a_data->latest_prio * -100000 + a_data->hit_count;
	unsigned long b_score = b_data->latest_prio * -100000 + b_data->hit_count;
	
	return b_score > a_score ? 1 : 0;
}


/**
 * 
 */
void strip_port(char *address) {
	char *p = strrchr(address, ':');
	if( p != NULL ) {
		p[0] = '\0';
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
		//                                                                                                                  ^ p3


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

			int prio = 99;
			char *p3 = strstr(line, "Priority:");
			if( p3 != NULL ) {
				prio = atoi(p3 + 9);
			}

			if( strip_src_port ) {
				strip_port(host_src);
			}

			char host_dst[128];
			memset(host_dst, 0, sizeof(host_dst));
			strncpy(host_dst, p2 + 4, line + strlen(line) - p2 -5);

			char message[1024];
			memset(message, 0, sizeof(message));
			strncpy(message, line, p0 - line -1);

			struct linked_item *item;
			struct event_data *item_data;

			item = find_item(host_src, host_dst);
			if( item == NULL ) {
				item = create_item(host_src, host_dst);
			}

			item_data = (struct event_data*) item->data;
			strcpy(item_data->latest_proto, proto);
			strcpy(item_data->latest_message, message);
			item_data->hit_count++;
			item_data->latest_prio = prio;

			data = linked_item_sort(data, &compare);
			print_data();
		}
	}

	if (feof(cmd)) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
