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
 * This program starts snort an reads output.
 * Creates a sorted list of hosts and related alerts
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
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "lib/linked_items.c"
#include "lib/net_util.c"
#include "lib/net_srv_util.c"
#include "lib/httpd_util.c"
#include "lib/html_convert.c"

char *snort_command = "/usr/sbin/snort -d -i %s -h %s -A console -c %s";
char *interface = NULL;
char *home_network = NULL;
char *snort_conf = "/etc/snort/snort.conf";

int print_max_items = 20;
int strip_src_port = 0;
int verbosity = 0;

char *server_ip = NULL;
int server_port = 8003;
int server_socket = 0;

struct event_data {
	char host_src[255];
	char host_dst[255];
	int hit_count;
	char latest_proto[10];
	char latest_message[1024];
	int latest_prio;
};

struct linked_item *data = NULL;
pthread_mutex_t update_data_mutex;

struct linked_item *out = NULL;

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

	struct linked_item *item = linked_item_create(linked_item_last(data));
	if( data == NULL ) {
		data = item;
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

	const char *options = "i:h:s:p:vm";
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

		case 's':
			server_ip = optarg;
			break;

		case 'p':
			server_port = atoi(optarg);
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
void create_out() {

	if( out != NULL ) {
		linked_item_free(out);
	}

	out = linked_item_appends(NULL, "SRC HOST\tDST HOST\tPROTO\n");
	struct linked_item *curr_out = out;

	struct linked_item *curr = data;
	struct event_data *curr_data;
	char buff[1048];
	int num = 0;
	while (curr != NULL && num < print_max_items) {

		curr_data = (struct event_data*) curr->data;

		sprintf(buff, "%s\t%s\t(%s)\n", curr_data->host_src, curr_data->host_dst, curr_data->latest_proto);
		curr_out = linked_item_appends(curr_out, buff);

		sprintf(buff, "\tHits: %i\n", curr_data->hit_count);
		curr_out = linked_item_appends(curr_out, buff);

		sprintf(buff, "\t%s\n", curr_data->latest_message);
		curr_out = linked_item_appends(curr_out, buff);

		curr = curr->next;
		num++;
	}

	curr_out = linked_item_appends(curr_out, "\n");
	sprintf(buff, "Count: %i\n", linked_item_count(data));
	linked_item_appends(curr_out, buff);
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

	pthread_mutex_lock(&update_data_mutex);

	struct linked_item *curr_out = out;
	while (curr_out != NULL) {
		printf("%s", (char*)curr_out->data);
		curr_out = curr_out->next;
	}

	pthread_mutex_unlock(&update_data_mutex);
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
void handle_request(int client_socket, struct sockaddr_in *client_address) {

	if( init_response(client_socket, "text/html", verbosity) < 0 ) {
		return;
	}

	char *s = "<!DOCTYPE html>\n";
	send(client_socket, s, strlen(s), 0);
	s = "<html><head><style>*{font-family: sans-serif;}td{padding:4px;border-bottom:1px solid black}</style></head><body>\n";
	send(client_socket, s, strlen(s), 0);

	pthread_mutex_lock(&update_data_mutex);

	struct linked_item *curr_out = out;
	while (curr_out != NULL) {
		html_append_text((char*)curr_out->data);
		curr_out = curr_out->next;
	}
	html_finish();

	struct linked_item *i = html;
	while( i != NULL ) {
		send(client_socket, (char*)i->data, strlen(i->data), 0);
		i = i->next;
	}
	linked_item_free(html);
	html = NULL;


	pthread_mutex_unlock(&update_data_mutex);

	s = "</body></html>\n";
	send(client_socket, s, strlen(s), 0);
}

/**
 * 
 */
void *start_accept_loop() {
	accept_loop(server_socket, &handle_request);
	return NULL;
}

/**
 * 
 */
void sig_handler(int signum) {
	printf("Caught signal %d\n", signum);
}

/**
 * 
 */
void usage_message() {
	printf("Usage: snort-scan-util [-s] [-v] -h <home network> -i <interface>\n");
}

/**
 * 
 */
int main(int argc, char *argv[]) {

	configure(argc, argv);
	if (interface == NULL) {
		fprintf(stderr, "Please select an interface (-i)\n");
		usage_message();
		exit (EX_USAGE);
	}
	if (home_network == NULL) {
		fprintf(stderr, "Please select a home network (-h)\n");
		usage_message();
		exit (EX_USAGE);
	}

	if( server_ip != NULL ) {
		server_socket = create_server_socket(server_ip, server_port);
		if( server_socket < 0 ) {
			fprintf(stderr, "Failed to create server\n");
			exit (EX_IOERR);
		}
		pthread_t thread_id;
		pthread_create(&thread_id, NULL, start_accept_loop, NULL);
	}

	signal(SIGPIPE, &sig_handler);


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

			pthread_mutex_lock(&update_data_mutex);

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

			create_out();

			pthread_mutex_unlock(&update_data_mutex);

			if( server_ip == NULL ) {
				print_data();
			}
		}
	}

	if (feof(cmd)) {
		pclose(cmd);
	} else {
		fprintf(stderr, "Broken pipe: %s", cmdString);
	}

}
