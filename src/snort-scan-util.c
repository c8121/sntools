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
int timespan_seconds = 3600;

char *server_ip = NULL;
int server_port = 8003;
int server_socket = 0;

struct hit_count {
	struct linked_item list;
	time_t ts;
	unsigned int hits;
};

struct event_data {
	struct linked_item list;
	char host_src[255];
	char host_dst[255];
	char latest_proto[10];
	char latest_message[1024];
	int latest_prio;
	struct hit_count *hit_count;
};


struct out_data {
	struct linked_item list;
	char s[512];
};

struct event_data *data = NULL;
pthread_mutex_t update_data_mutex;

struct out_data *out = NULL;

/**
 * 
 */
struct event_data* find_item(char *host_src, char *host_dst) {
	struct event_data *item = data;
	while (item != NULL) {
		if (strcmp(item->host_src, host_src) == 0) {
			if (strcmp(item->host_dst, host_dst) == 0) {	
				return item;
			}
		}
		item = (struct event_data*) item->list.next;
	}

	return NULL;
}

/**
 * 
 */
struct event_data* create_item(char *host_src, char *host_dst) {

	struct event_data *item = linked_item_create(linked_item_last(data), sizeof(struct event_data));
	if( data == NULL ) {
		data = item;
	}

	strcpy(item->host_src, host_src);
	strcpy(item->host_dst, host_dst);
	item->latest_proto[0] = '\0';
	item->latest_message[0] = '\0';
	item->latest_prio = 99;

	item->hit_count = NULL;

	return item;
}

/**
 * 
 */
void add_hit(struct event_data *item, int c, time_t ts) {

	struct hit_count *hit_count;
	if( item->hit_count == NULL ) {
		item->hit_count = linked_item_create(NULL, sizeof(struct hit_count));
		hit_count = item->hit_count;
	} else {
		hit_count = linked_item_create(linked_item_last(item->hit_count), sizeof(struct hit_count));
	}

	hit_count->hits = c;
	hit_count->ts = ts;
}

/**
 * 
 */
unsigned int sum_hits(struct event_data *item) {

	if( item->hit_count == NULL ) {
		return 0;
	}

	unsigned int sum = 0;
	struct hit_count *curr = item->hit_count;
	while( curr != NULL ) {
		sum += curr->hits;
		curr = (struct hit_count*) curr->list.next;
	}

	return sum;
}

/**
 * 
 */
void remove_hit_count_before(time_t before) {

	struct event_data *item = data;
	while (item != NULL) {
		struct hit_count *hit_count = item->hit_count;
		while( hit_count != NULL ) {
			if( hit_count->ts < before ) {
				linked_item_remove(hit_count);
				if( item->hit_count == hit_count ) {
					item->hit_count = (struct hit_count*) hit_count->list.next;
				}
				void *tmp = hit_count->list.next;
				free(hit_count);
				hit_count = tmp;
			} else {
				return; //items appear in order, it is safe to exit
			}
		}

		item = (struct event_data *) item->list.next;
	}
}

/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:h:s:p:t:vm";
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

		case 't':
			timespan_seconds = atoi(optarg);
			printf("Will keep data %i seconds\n", timespan_seconds);
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
struct out_data* append_out(struct out_data *prev, char *s) {
	struct out_data *item = linked_item_create(prev, sizeof(struct out_data));
	strcpy(item->s, s);
	return item;
}

/**
 * 
 */
void create_out() {

	if( out != NULL ) {
		linked_item_free(out, NULL);
	}

	char buff[1048];

	out = linked_item_create(NULL, sizeof(struct out_data));
	time_t now = time(NULL);
	sprintf(out->s, "Snort scan: timespan=%i sec, current time=%s\n", timespan_seconds, asctime(localtime(&now)));


	struct out_data *curr_out = out;
	curr_out = append_out(curr_out, "\n");
	curr_out = append_out(curr_out, "SRC HOST\tDST HOST\tPROTO\n");

	struct event_data *curr = data;
	int num = 0;
	while (curr != NULL && num < print_max_items) {

		sprintf(buff, "%s\t%s\t(%s)\n", curr->host_src, curr->host_dst, curr->latest_proto);
		curr_out = append_out(curr_out, buff);

		sprintf(buff, "\tHits: %i\n", sum_hits(curr));
		curr_out = append_out(curr_out, buff);

		sprintf(buff, "\t%s\n", curr->latest_message);
		curr_out = append_out(curr_out, buff);

		curr = (struct event_data *) curr->list.next;
		num++;
	}

	curr_out = append_out(curr_out, "\n");
	sprintf(buff, "Count: %i\n", linked_item_count(data));
	append_out(curr_out, buff);
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

	struct out_data *curr_out = out;
	while (curr_out != NULL) {
		printf("%s", curr_out->s);
		curr_out = (struct out_data *) curr_out->list.next;
	}

	pthread_mutex_unlock(&update_data_mutex);
}

/**
 * comparison of prio and hits, used by linked_item_sort
 */
int compare(void *a, void *b) {
	struct event_data *a_data = a;
	struct event_data *b_data = b;
	unsigned long a_score = a_data->latest_prio * -100000 + sum_hits(a_data);
	unsigned long b_score = b_data->latest_prio * -100000 + sum_hits(b_data);

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

	struct out_data *curr_out = out;
	while (curr_out != NULL) {
		html_append_text(curr_out->s);
		curr_out = (struct out_data*) curr_out->list.next;
	}
	html_finish();

	struct html_line *i = html;
	while( i != NULL ) {
		send(client_socket, i->s, strlen(i->s), 0);
		i = (struct html_line *)i->list.next;
	}
	linked_item_free(html, NULL);
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

		time_t now = time(NULL);

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

			struct event_data *item;

			item = find_item(host_src, host_dst);
			if( item == NULL ) {
				item = create_item(host_src, host_dst);
			}

			strcpy(item->latest_proto, proto);
			strcpy(item->latest_message, message);
			item->latest_prio = prio;

			remove_hit_count_before(now - timespan_seconds);
			add_hit(item, 1, now);

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
