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
 * Very simple util to convert text to html
 */

struct linked_item *html = NULL;
struct linked_item *html_last = NULL;

#define UNDEFINED 0
#define TEXT 1
#define TABLE 2

int html_curr_paragraph_type = UNDEFINED;




/**
 * 
 */
void _html_append(char *s, int max) {

	if( html_last == NULL && html != NULL ) {
		//Begin new conversion
		linked_item_free(html);
		html = NULL;
	}

	if( html == NULL ) {
		html = malloc(sizeof(struct linked_item));
		html_last = html;
	} else {
		struct linked_item *next = malloc(sizeof(struct linked_item));
		html_last->next = next;
		html_last = next;
	}

	html_last->next = NULL;
	
	if( max < 0 ) {
		html_last->data = malloc(strlen(s)+1);
		strcpy(html_last->data, s);
	} else {
		html_last->data = malloc(max+1);
		memset(html_last->data, 0, max+1);
		strncpy(html_last->data, s, max);
	}
}

/**
 * 
 */
void _html_finish_block() {
	if( html_curr_paragraph_type == TEXT) {
		_html_append("</p>", -1);
	}else if( html_curr_paragraph_type == TABLE) {
		_html_append("</table>", -1);
	}
}



/**
 * 
 */
void html_append_text(char *text) {

	if( strchr(text, '\t') != NULL ) {

		//TABLE line
		if( html_curr_paragraph_type != TABLE ) {
			_html_finish_block();
			_html_append("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">", -1);
		}

		html_curr_paragraph_type = TABLE;
		_html_append("<tr><td>", -1);

		char *s = text;
		char *p = text;
		while (p[0] != '\0') {
			if( p[0] == '\t' ) {
				if( s < p-1 ) {
					_html_append(s, p-s);
				}
				_html_append("</td><td>", -1);
				s = p+1;
			}
			p++;
		}
		if( s < p-1 ) {
			_html_append(s, -1);
		}

		_html_append("</td></tr>", -1);
	} else {

		//TEXT line
		if( html_curr_paragraph_type != TEXT ) {
			_html_finish_block();
			_html_append("<p>", -1);
		}

		html_curr_paragraph_type = TEXT;
		_html_append(text, -1);
	}
}

/**
 * 
 */
void html_finish() {
	_html_finish_block();
	html_last = NULL;
	html_curr_paragraph_type = UNDEFINED;
}


