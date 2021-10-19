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
 * This program starts tcpdump an reads output.
 * Creates a sorted list of hosts and summary of trasferred bytes 
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

char *tcpdump_command = "tcpdump -ni %s";
char *interface = NULL;

int verbosity = 0;
int print_max_items = 20;
int ignore_direction = 0;
int strip_port_above = 1024;
int human_readable = 0;
int timespan_seconds = 3600;

int print_interval_seconds = 3;
time_t last_print_data = 0;

char *server_ip = NULL;
int server_port = 8002;
int server_socket = 0;

struct byte_count {
	struct linked_item list;
	time_t ts;
	unsigned long bytes;
};

struct host_data {
	struct linked_item list;
	char host_a[255];
	char host_b[255];
	struct byte_count *byte_count;
};

struct out_data {
	struct linked_item list;
	char s[512];
};

struct host_data *data = NULL;
pthread_mutex_t update_data_mutex;

struct out_data *out = NULL;

/**
 * 
 */
struct host_data* find_item(char *host_a, char *host_b) {
	struct host_data *item = data;
	while (item != NULL) {
		if (strcmp(item->host_a, host_a) == 0) {
			if (strcmp(item->host_b, host_b) == 0) {	
				return item;
			}
		}
		item = (struct host_data *)item->list.next;
	}

	return NULL;
}

/**
 * 
 */
struct host_data* create_item(char *host_a, char *host_b) {

	struct host_data *item = linked_item_create(linked_item_last(data), sizeof(struct host_data));
	if( data == NULL ) {
		data = item;
	}

	strcpy(item->host_a, host_a);
	strcpy(item->host_b, host_b);
	item->byte_count = NULL;

	return item;
}

/**
 * 
 */
void add_bytes(struct host_data *item, unsigned long b, time_t ts) {

	struct byte_count *byte_count;
	if( item->byte_count == NULL ) {
		item->byte_count = linked_item_create(NULL, sizeof(struct byte_count));
		byte_count = data->byte_count;
	} else {
		byte_count = linked_item_create(linked_item_last(item->byte_count), sizeof(struct byte_count));
	}

	byte_count->bytes = b;
	byte_count->ts = ts;
}

/**
 * 
 */
unsigned long sum_bytes(struct host_data *item) {

	if( item->byte_count == NULL ) {
		return 0;
	}

	unsigned long sum = 0;
	struct byte_count *curr = item->byte_count;
	while( curr != NULL ) {
		sum += curr->bytes;
		curr = (struct byte_count*) curr->list.next;
	}

	return sum;
}

/**
 * 
 */
void remove_byte_count_before(time_t before) {

	struct host_data *item = data;
	while (item != NULL) {
		struct byte_count *byte_count = item->byte_count;
		while( byte_count != NULL ) {
			if( byte_count->ts < before ) {
				linked_item_remove(byte_count);
				if( item->byte_count == byte_count ) {
					item->byte_count = (struct byte_count*) byte_count->list.next;
				}
				void *tmp = byte_count->list.next;
				free(byte_count);
				byte_count = tmp;
			} else {
				byte_count = (struct byte_count*) byte_count->list.next;
			}
		}

		item = (struct host_data *) item->list.next;
	}
}


/**
 * Read command line arguments and configure application
 */
void configure(int argc, char *argv[]) {

	const char *options = "i:m:n:s:p:t:vxh";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {

		case 'i':
			interface = optarg;
			printf("Select interface: %s\n", interface);
			break;

		case 'm':
			strip_port_above = atoi(optarg);
			printf("Will not show ports above %i\n", strip_port_above);
			break;

		case 'n':
			print_max_items = atoi(optarg);
			printf("Will show max. %i items\n", print_max_items);
			break;

		case 'x':
			ignore_direction = 1;
			break;

		case 'h':
			human_readable = 1;
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
void readable_bytes(unsigned long bytes, char *buf) {
	int i = 0;
	double result = bytes;
	const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	while (result > 1024) {
		result /= 1024;
		i++;
	}
	sprintf(buf, "%.2f %s", result, units[i]);
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

	char buff[1024];

	out = linked_item_create(NULL, sizeof(struct out_data));
	time_t now = time(NULL);
	sprintf(out->s, "Hostwatch: timespan=%i sec, current time=%s\n", timespan_seconds, asctime(localtime(&now)));

	struct out_data *curr_out = out;
	curr_out = append_out(curr_out, "\n");
	curr_out = append_out(curr_out, "HOST A\tHOST B\tBYTES (Packet count)\n");

	struct host_data *curr = data;
	char bytes_s[32];
	int num = 0;
	while (curr != NULL && num < print_max_items) {

		if( human_readable ) {
			readable_bytes(sum_bytes(curr), bytes_s);
		} else {
			sprintf(bytes_s, "%lu", sum_bytes(curr));
		}

		sprintf(buff, "%s\t%s\t%s (%i)\n", curr->host_a, curr->host_b, bytes_s, linked_item_count(curr->byte_count));
		curr_out = append_out(curr_out, buff);


		curr = (struct host_data *) curr->list.next;
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
		curr_out = (struct out_data*) curr_out->list.next;
	}

	pthread_mutex_unlock(&update_data_mutex);
}


/**
 * comparison of bytes, used by linked_item_sort
 */
int compare(void *a, void *b) {

	struct host_data *ha = (struct host_data*) a;
	struct host_data *hb = (struct host_data*) b;

	if( sum_bytes(hb) > sum_bytes(ha) )
		return 1;
	else
		return 0;
}


/**
 * 
 */
void strip_port(char *address) {
	char *p = strrchr(address, '.');
	if( p != NULL ) {
		int port = atoi(p+1);
		if( port > strip_port_above ) {
			p[0] = '\0';
		}
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
		curr_out = (struct out_data *) curr_out->list.next;
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
	printf("Usage: hostwatch [-x] [-v] [-m <port>]  [-n <hosts-to-print>] [-s <ip>] [-p <port>] [-h] -i <interface>\n");
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
		struct host_data *item;

		time_t now = time(NULL);

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

						if( strip_port_above > 0 ) {
							strip_port(fromAddress);
							strip_port(toAddress);
						}

						unsigned long bytes = atoi(p3 + 7);

						if (verbosity > 1) {
							printf("%s -> %s: %lub\n", fromAddress, toAddress,
									bytes);
						}

						pthread_mutex_lock(&update_data_mutex);

						item = find_item(fromAddress, toAddress);
						if (item == NULL) {
							if( ignore_direction ) {
								item = find_item(toAddress, fromAddress);
							}

							if (item == NULL) {
								item = create_item(fromAddress, toAddress);
							}
						}

						remove_byte_count_before(now - timespan_seconds);
						add_bytes(item, bytes, now);
						if (verbosity > 0) {
							printf("Update: %s -> %s\n", item->host_a, item->host_b);
						}

						data = linked_item_sort(data, &compare);

						create_out();

						pthread_mutex_unlock(&update_data_mutex);

						if( server_ip == NULL ) {
							if( (now - last_print_data) > (print_interval_seconds) ) {
								print_data();
								last_print_data = now;
							}
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
