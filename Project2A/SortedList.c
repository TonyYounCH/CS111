/*
NAME: Changhui Youn
EMAIL: tonyyoun2@gmail.com
ID: 304207830
*/


#include <stdio.h>
#include <string.h>
#include <sched.h>
#include "SortedList.h"


void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	if (list == NULL || element == NULL) {
		return;
	}
	SortedListElement_t *ptr = list->next;
	if (opt_yield & INSERT_YIELD){
		sched_yield();
	}
	while(ptr != list){
		if (strcmp(element->key, ptr->key) <= 0){
			break;
		}
		ptr = ptr->next;
	}
	element->prev = ptr->prev;
	element->next = ptr;
	ptr->prev->next = element;
	ptr->prev = element;
	return;
}

int SortedList_delete( SortedListElement_t *element) {
	if ((element == NULL) || (element->next->prev != element) || (element->prev->next != element)) {
		  return 1;
	}
	if (opt_yield & DELETE_YIELD) {
		sched_yield();
	}
	element->next->prev = element->prev;
	element->prev->next = element->next;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	if (list == NULL || key == NULL) return NULL;
	SortedListElement_t *ptr = list->next;
	if (opt_yield & LOOKUP_YIELD) {
		sched_yield();
	}
	while(ptr != list) {
		if (strcmp(ptr->key, key) == 0){
			return ptr;
		}
		ptr = ptr->next;
	}
	return NULL;
}

int SortedList_length(SortedList_t *list){
	if (list == NULL) {
		return -1;
	}

	SortedListElement_t *ptr = list->next;
	int length = 0;
	if (opt_yield & LOOKUP_YIELD) {
		sched_yield();
	}
	while(ptr != list) {
		if ((ptr == NULL) || (ptr->next == NULL) || (ptr->prev == NULL)) {
			return -1;
		}
		if ((ptr->next->prev != ptr) || (ptr->prev->next != ptr)) {
			return -1;
		}
		ptr = ptr->next;
		length++;
	}
	return length;
}
