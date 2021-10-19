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
 * Tiny and very simple text to html converter.
 * Suitable for output of sntools only
 */

/*
 * Author: christian c8121 de
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../lib/linked_items.c"
#include "../lib/html_convert.c"

/**
 * 
 */
void show_current(struct html_line *start) {
	struct html_line *curr = start;
	while( curr != NULL ) {
		printf("%s", curr->s);
		curr = (struct html_line *) curr->list.next;
	}
	printf("EOL\n");
}

/**
 * 
 */
int main(int argc, char *argv[]) {


	html_append_text("Hello World\n");
	html_finish();
	show_current(html);
	
	html_append_text("Hello World\n");
	html_append_text("A\tB\tC\n");
	html_append_text(" 1\t[2]\t 3\n");
	html_append_text("\t\t\n");
	html_append_text("END...\n");
	html_finish();
	show_current(html);
}


