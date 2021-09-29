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

#define LINE_BUF_SIZE 512

char buf[LINE_BUF_SIZE];
char *bufp;

/**
 * Print table, read more data from stdin, return if no tab found in line
 */
void print_table() {

	printf("<table border=\"0\">\n");
	
	do {

		printf("<tr><td>");
		
		int n = strlen(buf);
		for( int i=0 ; i < n ; i++ ) {
			if( buf[i] == '\t' ) {
				printf("</td><td>");
			} else if( buf[i] != '\n' ) {
				printf("%c", buf[i]);
			}
		}
		
		printf("</td></tr>\n");
		
		if( (bufp = fgets(buf, LINE_BUF_SIZE, stdin)) == NULL )
			break;
		
	} while( index(buf, '\t') != NULL );
	
	
	printf("</table>\n");
	
}

/**
 * 
 */
void print_paragraphs() {
	
	printf("<p>\n");
	
	do {
		printf("%s", buf);
		
		if( (bufp = fgets(buf, LINE_BUF_SIZE, stdin)) == NULL )
			break;
		
		if( buf[0] == '\n' ) {
			printf("</p><p>\n");
		} else if( index(buf, '\t') != NULL ) {
			printf("</p>\n");
			print_table();
			return;
		} else {
			printf("<br>\n");
		}
	} while(1);
	
	printf("</p>\n");
}

/**
 * Tiny and very simple text to html converter.
 * Suitable for output of sntools only
 */
int main(int argc, char *argv[]) {
	
	printf("<!DOCTYPE html>\n");
	printf("<html><head><style>*{font-family: sans-serif;}td{padding:4px;border-bottom:1px solid black}</style></head><body>");

	while( (bufp = fgets(buf, LINE_BUF_SIZE, stdin)) != NULL ) {
		if( index(buf, '\t') != NULL ) {
			print_table();
		}
		
		if( bufp != NULL ) {
			print_paragraphs();
		}
	}

	printf("</body></html>");
}
