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
