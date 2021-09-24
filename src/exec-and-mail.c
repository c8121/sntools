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

char *command;
char *smtpHost;
int smtpPort;
char *envFrom;
char *envTo;
char *heloName;
char *subject = "Output from command";

int waitTimeout = 0;

int verbosity = 0;


pthread_mutex_t handle_data_mutex;


typedef struct message_line {
    char *line;
    struct message_line *next;
} message_line_t;

/**
 *
 */
int message_line_count(message_line_t *start) {
    int c = 0;
    message_line_t *curr = start;
    while( curr != NULL ) {
        c++;
        curr = curr->next;
    }
    return c;
}



/**
 * Free memory of whole chain
 */
void message_line_free(message_line_t *start) {
    if( start == NULL ) {
        return;
    }
    message_line_free(start->next);

    free(start->line);
    free(start->next);
}

message_line_t *buffer = NULL;
int bufferMaxLines = 100;


/**
 *
 */
int resolve(const char *serverName, char *resolvedName, int size) {

    struct addrinfo hints;
    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;


    struct addrinfo *info;
    int r = getaddrinfo(serverName, NULL, &hints, &info);
    if( r != 0 ) {
        fprintf(stderr, "Cannot resolve host: %s\n", serverName);
        return r;
    }

    struct addrinfo *result = info;
    while( result ) {

        void *ptr;
        if( result->ai_family == AF_INET ) {
            ptr = &((struct sockaddr_in *) result->ai_addr)->sin_addr;
        } else if( result->ai_family == AF_INET6 ) {
            ptr = &((struct sockaddr_in6 *) result->ai_addr)->sin6_addr;
        }

        inet_ntop( result->ai_family, ptr, resolvedName, size );

        result = result->ai_next;
    }

    return 0;
}


/**
 *
 */
int open_socket(const char *serverName, int port) {

    char ip[255];
    resolve(serverName, ip, sizeof(ip));
    if( ip[0] == '\0' ) {
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( sockfd < 0 ) {
        fprintf(stderr, "Failed to create socket\n");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) {
        fprintf(stderr, "Invalid address/ Address not supported: %s\n", serverName);
        close(sockfd);
        return -1;
    }

    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection failed: %s:%i\n", serverName, port);
        close(sockfd);
        sockfd = -1;
    }

    return sockfd;
}

/**
 * Read and check server response.
 * Return -1 on error response, 0 otherwise
 */
int smtp_read_line(int socket) {

    char response[255];
    char c;
    int i=0;
    while( 1 ) {

        int n = read(socket, &c, 1);
        if( n < 1 ) {
            return -1;
        }

        if( c == '\n' ) {

            if( i > 0 && (response[0] == '2' || response[0] == '3') ) {
                return 0;
            } else {
                fprintf(stderr, "Got error response: %s\n", response);
                return -1;
            }
        }

        if( i < 254 ) {
            response[i] = c;
            response[i+1] ='\0';
            i++;
        }
    }

}

/**
 *
 */
int send_buffer() {

    int socket = open_socket(smtpHost, smtpPort);
    char buf[1024];
    if( socket > 0 ) {

        //Read SMTP Greeting
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        snprintf(buf, sizeof(buf), "helo %s\r\n", heloName);
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        snprintf(buf, sizeof(buf), "mail from: <%s>\r\n", envFrom);
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        snprintf(buf, sizeof(buf), "rcpt to: <%s>\r\n", envTo);
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        snprintf(buf, sizeof(buf), "data\r\n");
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        // Send header
        snprintf(buf, sizeof(buf), "Subject: %s\r\n", subject);
        write(socket, buf, strlen(buf));
        snprintf(buf, sizeof(buf), "From: <%s>\r\n", envFrom);
        write(socket, buf, strlen(buf));
        snprintf(buf, sizeof(buf), "To: <%s>\r\n", envTo);
        write(socket, buf, strlen(buf));

        // Start body
        snprintf(buf, sizeof(buf), "\r\n");
        write(socket, buf, strlen(buf));

        // Send body which has been read above
        message_line_t *curr = buffer;
        while( curr != NULL ) {
            if (strlen(curr->line) > 1 && curr->line[0] == '.' && (curr->line[1] == '\r' || curr->line[1] == '\n')) {
                write(socket, ".", 1);
            }
            snprintf(buf, sizeof(buf), "%s", curr->line);
            write(socket, buf, strlen(buf));
            curr = curr->next;
        }

        snprintf(buf, sizeof(buf), "\r\n.\r\n");
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        snprintf(buf, sizeof(buf), "quit\r\n");
        write(socket, buf, strlen(buf));
        if( smtp_read_line(socket) != 0 ) {
            return -1;
        }

        close(socket);
    }

    return 0;
}


/**
*
*/
void handle_data(int force) {

    if( verbosity > 0 ) {
        printf("check if data should be sent (force=%i)\n", force);
    }

    pthread_mutex_lock(&handle_data_mutex);
    if( buffer == NULL ) {
        pthread_mutex_unlock(&handle_data_mutex);
        return;
    }

    if( force || message_line_count(buffer) >= bufferMaxLines ) {

        if( verbosity > 1 ) {
            message_line_t *out = buffer;
            while( out != NULL ) {
                printf("SEND> %s", out->line);
                out = out->next;
            }
        }

        send_buffer();

        message_line_free(buffer);
        buffer = NULL;
    }

    pthread_mutex_unlock(&handle_data_mutex);

}

void *timer() {
    while( 1 ) {
        sleep(waitTimeout);
        handle_data(1);
    }
}

void create_timer() {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, timer, NULL);
}


/**
 * Read command line arguments and configure application
 * Create data directoy
 */
void configure(int argc, char *argv[]) {


	// -- Read CLI arguments -------

	const char *options = "s:c:t:vv";
	int c;

	while ((c = getopt(argc, argv, options)) != -1) {
		switch(c) {

		    case 's':
                subject = optarg;
                break;

            case 't':
                waitTimeout = atoi(optarg);
                printf("Wait timeout: %i seconds\n", waitTimeout);
                break;

            case 'c':
                bufferMaxLines = atoi(optarg);
                printf("Set buffer size: %i lines\n", bufferMaxLines);
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
void usage_message() {
    printf("Usage: exec-and-mail [-c buffer-line-count] [-t wait-timeout-seconds] [-v] [-s subject] <host> <port> <from> <to> \"<command>\"\n");
}


/**
 * 
 */
int main(int argc, char *argv[]) {

    configure(argc, argv);

    if (argc - optind +1 < 6) {
        fprintf(stderr, "Missing arguments\n");
        usage_message();
        exit(EX_USAGE);
    }

    // SMTP Server
    smtpHost = argv[optind];
    smtpPort = atoi(argv[optind+1]);
    
    // Check sender and recipient
    envFrom = argv[optind+2];
    envTo = argv[optind+3];
    if( envFrom[0] == '\0' || envTo[0] == '\0' ) {
        fprintf(stderr, "Parameter 'from' and 'to' cannot be empty\n");
        usage_message();
        exit(EX_USAGE);
    }
    
    // Read hostname
    char buf[128];
    gethostname(buf, sizeof(buf));
    heloName = buf;

    //Command
    command = argv[optind+4];
    if( command[0] == '\0' ) {
        fprintf(stderr, "No command given\n");
        usage_message();
        exit(EX_USAGE);
    }

    if( waitTimeout > 0 ) {
        create_timer();
    }

    printf("Execute: %s\n", command);

    FILE *cmd = popen(command, "r");
    if( cmd == NULL ) {
        fprintf(stderr, "Failed to execute: %s", command);
        exit(EX_IOERR);
    }

    message_line_t *readInto = NULL;

    char line[1024];
    while( fgets(line, sizeof(line), cmd) ) {

        if( verbosity > 0 ) {
            printf("command> %s", line);
        }

        pthread_mutex_lock(&handle_data_mutex);

        if( buffer == NULL ) {
            buffer = malloc(sizeof(message_line_t));
            readInto = buffer;
        } else {
            readInto->next = malloc(sizeof(message_line_t));
            readInto = readInto->next;
        }

        readInto->next = NULL;

        int len = strlen(line);
        readInto->line = malloc(len+1);
        strcpy(readInto->line, line);

        pthread_mutex_unlock(&handle_data_mutex);

        handle_data(0);
    }

    if( feof(cmd) ) {
        pclose(cmd);
    } else {
        fprintf(stderr, "Broken pipe: %s", command);
    }

    handle_data(1);
}
