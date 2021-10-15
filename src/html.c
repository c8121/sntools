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
 * Author: christian c8121 de
 */

/*
 * Tiny and very simple text to html converter.
 * Suitable for output of sntools only
 */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "lib/linked_items.c"
#include "lib/html_convert.c"

#define LINE_BUF_SIZE 512


/**
 * Tiny and very simple text to html converter.
 * Suitable for output of sntools only
 */
int main(int argc, char *argv[]) {
	
	printf("<!DOCTYPE html>\n");
	printf("<html><head><style>*{font-family: sans-serif;}td{padding:4px;border-bottom:1px solid black}</style></head><body>\n");

	char buf[LINE_BUF_SIZE];
	while( fgets(buf, LINE_BUF_SIZE, stdin) != NULL ) {
		html_append_text(buf);
	}
	html_finish();
	
	struct linked_item *i = html;
	while( i != NULL ) {
		printf("%s", (char*)i->data);
		i = i->next;
	}
	
	linked_item_free(html);
	html = NULL;

	printf("</body></html>");
}
