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


struct linked_item {
	struct linked_item *prev;
	struct linked_item *next;
};

/**
 * Create a new linked item, instert after prev, set prev & next
 */
void __linked_item_append(struct linked_item *prev, struct linked_item *item) {

	item->prev = NULL;
	item->next = NULL;

	if( prev != NULL ) {
		if( prev->next != NULL ) {
			item->next = prev->next;
			prev->next->prev = item;
		}
		prev->next = item;
		item->prev = prev;
	}
}

void linked_item_append(void *prev, void *item) {
	__linked_item_append(prev, item);
}

/**
 * Create a new linked item, instert after prev, set prev & next
 */
void* linked_item_create(void *prev, size_t size) {

	struct linked_item *item = malloc(size);
	linked_item_append(prev, item);

	return item;
}



/**
 * Count all items beginning as given start (including start)
 */
int __linked_item_count(struct linked_item *start) {
	int c = 0;
	struct linked_item *curr = start;
	while( curr != NULL ) {
		c++;
		curr = curr->next;
	}
	return c;
}

int linked_item_count(void *start) {
	return __linked_item_count(start);
}

/**
 * Get last item from chain, starting at given point.
 */
void* __linked_item_last(struct linked_item *start) {
	struct linked_item *curr = start;
	while( curr != NULL ) {
		if( curr->next == NULL ) {
			return curr;
		}
		curr = curr->next;
	}
	return NULL;
}

void* linked_item_last(void *start) {
	return __linked_item_last(start);
}

/**
 * 
 */
void __linked_item_remove(struct linked_item *remove) {
	if( remove->prev != NULL ) {
		if( remove->next != NULL ) {
			remove->prev->next = remove->next;
			remove->next->prev = remove->prev;
		} else {
			remove->prev->next = NULL;
		}
	} else if( remove->next != NULL ) {
		remove->next->prev = NULL;
	}
}

/**
 * 
 */
void linked_item_remove(void *remove) {
	__linked_item_remove(remove);
}


/**
 * Free memory of whole chain
 */
void __linked_item_free(struct linked_item *start, void (*free_linked_item)(void* item)) {
	if( start == NULL ) {
		return;
	}

	if( free_linked_item != NULL ) {
		(*free_linked_item)(start);
	}

	if( start->next != NULL ) {    
		__linked_item_free(start->next, free_linked_item);
	}

	free(start);
}

void linked_item_free(void *start, void (*free_linked_item)(void* item)) {
	__linked_item_free(start, free_linked_item);
}


/**
 * Return new start item of chain
 */
void* __linked_item_sort(struct linked_item *start, int (*compare_function)(void *a, void *b)) {

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
					if( last == start ) {
						start = curr;
					}
					linked_item_remove(last);
					linked_item_append(curr, last);
					changed++;	
				}
			}
			last = curr;
			curr = curr->next;
		}

	} while( changed > 0 );

	return start;
}

void* linked_item_sort(void *start, int (*compare_function)(void *a, void *b)) {
	return __linked_item_sort(start, compare_function);
}