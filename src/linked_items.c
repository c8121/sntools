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


struct linked_item {
	void *data;
	struct linked_item *next;
};

/**
 * Count all items beginning as given start (including start)
 */
int linked_item_count(struct linked_item *start) {
	int c = 0;
	struct linked_item *curr = start;
	while( curr != NULL ) {
		c++;
		curr = curr->next;
	}
	return c;
}

/**
 * Get last item from chain, starting at given point.
 */
struct linked_item* linked_item_last(struct linked_item *start) {
	struct linked_item *curr = start;
	while( curr != NULL ) {
		if( curr->next == NULL ) {
			return curr;
		}
		curr = curr->next;
	}
	return NULL;
}

/**
 * Return new start item of chain
 */
struct linked_item* linked_item_insert_before(struct linked_item *insert, struct linked_item *before, struct linked_item *start) {
	struct linked_item *last = NULL;
	struct linked_item *curr = start;
	while( curr != NULL ) {
		if( curr == before ) {
			insert->next = before;
			if( last != NULL ) {
				last->next = insert;
				return start;
			} else {
				return insert;
			}
		}
		last = curr;
		curr = curr->next;
	}
	return start;
}

/**
 * Create (malloc) new linked_item, set data pointer to data, set append_to->next if append_to is not NULL
 */
struct linked_item* linked_item_append(struct linked_item *append_to, void *data) {
	struct linked_item *item = malloc(sizeof(struct linked_item));
	item->next = NULL;
	item->data = data;
	
	if( append_to != NULL ) {
		if( append_to->next == NULL ) {
			append_to->next = item;
		} else {
			fprintf(stderr, "append_to->next already set");
			return NULL;
		}
	}
	
	return item;
}

/**
 * Create (malloc) new linked_item, set data pointer to s (malloc & strcpy), set append_to->next if append_to is not NULL
 */
struct linked_item* linked_item_appends(struct linked_item *append_to, char *s) {
	char *data = malloc(strlen(s)+1);
	strcpy(data, s);
	
	return linked_item_append(append_to, data);
}

/**
 * Return new start item of chain
 */
struct linked_item* linked_item_remove(struct linked_item *remove, struct linked_item *start) {
	struct linked_item *last = NULL;
	struct linked_item *curr = start;
	while( curr != NULL ) {
		if( curr == remove ) {
			if( last != NULL ) {
				last->next = remove->next;
				return start;
			} else {
				return remove->next;
			}
		}
		last = curr;
		curr = curr->next;
	}
	return start;
}


/**
 * Free memory of whole chain
 */
void linked_item_free(struct linked_item *start) {
	if( start == NULL ) {
		return;
	}

	if( start->data != NULL ) {
		free(start->data);
	}

	if( start->next != NULL ) {    
		linked_item_free(start->next);
	}

	free(start);
}

/**
 * Return new start item of chain
 */
struct linked_item* linked_item_sort(struct linked_item *start, int (*compare_function)(struct linked_item *a, struct linked_item *b)) {

	if (start == NULL) {
		return start;
	}

	struct linked_item *last;
	struct linked_item *curr;

	int changed = 0;
	do {
		changed = 0;
		last = NULL;
		curr = start;

		while( curr != NULL ) {

			if( last != NULL ) {
				
				int comp = (*compare_function)(last, curr);
				
				if( comp > 0 ) {
					start = linked_item_remove(curr, start);
					start = linked_item_insert_before(curr, last, start);
					changed++;
				}
			}
			if( curr != NULL ) {
				last = curr;
				curr = curr->next;
			}
		}

	} while( changed > 0 );
	
	return start;
}
