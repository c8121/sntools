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


struct test_item {
	struct linked_item list;
	char payload[255];
	int sort;
};

/**
 * 
 */
int compare(void *a, void *b) {
	struct test_item *ta = a;
	struct test_item *tb = b;
	return tb->sort > ta->sort ? 1 : 0;
}

/**
 * 
 */
void free_item(void *item) {
	printf("Free %p\n", item);
}

/**
 * 
 */
void show_list(struct test_item *start) {
	struct test_item *curr = start;
	while( curr != NULL ) {
		printf("%p (prev=%p, next=%p)\n", curr, curr->list.prev, curr->list.next);
		printf("   \"%s\"\n", curr->payload);
		curr = (struct test_item *) curr->list.next;
	}
	printf("EOL\n");
}


/**
 * 
 */
int main(int argc, char *argv[]) {


	struct test_item *item1 = linked_item_create(NULL, sizeof(struct test_item));
	strcpy(item1->payload, "item1");
	show_list(item1);

	struct test_item *item3 = linked_item_create(item1, sizeof(struct test_item));
	strcpy(item3->payload, "item3");
	show_list(item1);

	struct test_item *item2 = linked_item_create(item1, sizeof(struct test_item));
	strcpy(item2->payload, "item2");
	show_list(item1);

	struct test_item *item4 = linked_item_create(item1, sizeof(struct test_item));
	strcpy(item4->payload, "item4");
	show_list(item1);
	
	linked_item_remove(item4);
	show_list(item1);
	
	linked_item_append(item3, item4);
	show_list(item1);
	
	printf("%i ITEMS IN LIST\n", linked_item_count(item1));
	printf("LAST ITEM: %p\n\n", linked_item_last(item1));
	
	struct test_item *sorted_list = linked_item_sort(item1, &compare);
	show_list(sorted_list);
	
	item1->sort = 1;
	item4->sort = 4;
	sorted_list = linked_item_sort(sorted_list, &compare);
	show_list(sorted_list);
	
	item3->sort = 3;
	sorted_list = linked_item_sort(sorted_list, &compare);
	show_list(sorted_list);
	
	item2->sort = 2;
	sorted_list = linked_item_sort(sorted_list, &compare);
	show_list(sorted_list);
	
	item1->sort = 5;
	sorted_list = linked_item_sort(sorted_list, &compare);
	show_list(sorted_list);
	
	
	
	linked_item_free(sorted_list, &free_item);
}


