/*
* Function implementations for Linked List
*/

#include "linkedList.h"

List_t* create_list() {
    List_t* list = malloc(sizeof(List_t));
    list->length = 0;
    list->head = NULL;
    return list;
}

// inserts node at front of list
void insertFront(List_t* list, void* val) {
    // create node with given data
    node_t* new_node = malloc(sizeof(node_t));
    new_node->prev = NULL;
    new_node->data = val;
    // check if list is empty
    if (list->length == 0) {
        
        list->head = new_node;
        new_node->next = NULL;
    // list is not empty
    } else {
        
        new_node->next = list->head;
        list->head = new_node;
    }
   
    list->length++;
}

// inserts node at rear of list
void insertRear(List_t* list, void* val) {
    // create node with given data
    node_t* new_node = malloc(sizeof(node_t));
    new_node->data = val;
    new_node->next = NULL;
    
    // check if list is empty
    if (list->length == 0) {
        list->head = new_node;
        new_node->prev = NULL;
    } else {
        node_t* current = list->head;
        // iterate to last node in list
        while (current->next != NULL) {
            current = current->next;
        }

        // add node to list
        current->next = new_node;
        new_node->prev = current;
    }
    list->length++;

   // fprintf(stderr, "item added to list\n");
}

// removes last element from list
void* removeRear(List_t* list) {
    void* dat = NULL;

    if (list->length == 0) {
        return NULL;
    } else if (list->length == 1) {
        node_t* temp = list->head;
        dat = temp->data;
        list->head = NULL;
        free(temp);
    } else {
        node_t* current = list->head;
        while (current->next->next != NULL) {
            current = current->next;
        }
        dat = current->next->data;
        free(current->next);
        current->next = NULL;
    }
    list->length--;
    return dat;
}

// removes head node
void* removeFront(List_t* list) {
    node_t** head = &(list->head);
    void* dat = NULL;
    node_t* next_node = NULL;

    if (list->length == 0) {
        return NULL;
    }

    next_node = (*head)->next;
    dat = (*head)->data;
    list->length--;

    node_t* tmp = *head;
    *head = next_node;
    free(tmp);

    return dat;
}

// removed node from list by index
void* removeByIndex(List_t* list, int index) {
    // error check
    if (list->length <= index) {
        return NULL;
    }

    if (list->length == 0) {
        return NULL;
    }

    void* dat = NULL;
    //if index = 0, remove front
    if (index == 0) {
        return removeFront(list);
    }
    if (index == (list->length - 1)) {
        return removeRear(list);
    }

    // iterate to desired index
    int i = 0;
    node_t* current = list->head;
    while (i != index) {
       current = current->next;
       i++;
    }

    dat = current->data;
    current->next->prev = current->prev;
    current->prev->next = current->next;

    free(current);
    list->length--;
    return dat;
}

// frees list and all contents
void deleteList(List_t* list) {
    if (list->length == 0) {
        return;
    }
    while (list->head != NULL) {
        removeRear(list);
    }
    list->length = 0;
}

// prints list for debugging
void printList(List_t* list) {
}



