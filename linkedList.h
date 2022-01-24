#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

/*
* Structure for node of doubly linked list
*
* data - pointer to data of node (user struct, etc)
* next/prev - pointers to next and prev pointers in list
*/
typedef struct node {
    void* data;
    struct node* next;
    struct node* prev;
} node_t;

/*
* Structure for doubly linked list
*
* head - pointer to head node
* length - length of list
*/
typedef struct list {
    node_t* head;
    int length;
} List_t;

/*
* Initializes list, sets head to null and all that
*/
List_t* create_list();

/*
* Functions to insert to rear of list
*
* @param list pointer to LinkedList struct
* @param val pointer to data
*/
void insertRear(List_t* list, void* val);
void insertFront(List_t* list, void* val);

/*
* Removes node by index / from rear / from front
*
* @param list pointer to LinkedList struct
* @param index index of node to remove
*/
void* removeByIndex(List_t* list, int index);
void* removeRear(List_t* list);
void* remmoveFront(List_t* list);

/*
* Frees all nodes in linkedList
*/
void deleteList(List_t* list);

/*
* Prints list contents for debugging
*/
void printList(List_t* list);

#endif
