
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include "list.h"


struct List *create_list() {

    struct List *lp = NULL;
    if ((lp = (struct List *)calloc(1, sizeof(struct List))) == NULL) {
        fprintf(stderr, "Failure to allocate sufficient resources\n");
        return NULL;
    }

    lp->capacity = 0;

    lp->head = NULL;
    lp->tail = NULL;

    return lp;
}


void echo_list(struct List *lp) {
   
    printf("\n");

    if (lp->capacity == 0) {
        printf("No remaining items\n");
        return;
    }

    struct Node *current = lp->head;
    while (current != NULL) {

        printf("%s: connfd: %d\n", current->nickname, current->connfd);

        current = current->next;
    }


}


void delete_list(struct List *lp) {
 
    //struct Node *current = lp->head;
    // start from the tail, remove items backwards
    while (lp->head != NULL && lp->capacity > 0) {  // Do not branch

        remove_node(lp, lp->head);

    }

    free(lp);
}

struct Node *insert_node(struct List *lp) {

    struct Node *node = NULL;
    if ((node = (struct Node *)calloc(1, sizeof(struct Node))) == NULL) {
        fprintf(stderr, "Failure to allocate sufficient resources\n");
        return NULL;
    }

    if (lp->head == NULL) {
        
        lp->head = node;
        lp->head->prev = NULL;
        lp->head->next = NULL;

        // Adding:       vvvv
        // List: NULL <- head -> NULL

    } else {
        
        // Adding:                vvvvv
        // List: head -> node1 -> node2 -> NULL
       
        struct Node *current = lp->head;
        while (current->next != NULL)
            current = current->next;

        current->next = node;
        node->prev = current;
        node->next = NULL;

        lp->tail = node;
    }

    lp->capacity += 1;

    return node;
}




void remove_node(struct List *lp, struct Node *node) {

    if (lp->head != node) {
           
        /* this is a subnode of some sort */

        if (node->prev != NULL) { 

            node->prev->next = node->next;

            if (node->next != NULL)
                node->next->prev = node->prev; 
                     
            struct Node *current = lp->head;
            while (current->next != NULL)
                 current = current->next;

            if (current != lp->head)
                lp->tail = current;   
            else
                lp->tail = NULL;

        }

    } else {

        /* this is the new head of the linked list */

        // remove: dead_node1 -> new_head
        // 

        if (lp->head->next != NULL) {
            lp->head = lp->head->next;
            
            if (lp->tail == lp->head) // last node remaining, remove the tail
                lp->tail = NULL;

        } else { // if we just deleted the head and theres nothing left
            lp->head = NULL;
            lp->tail = NULL;
        }
    }


    /* update the list tail */ 

    node->prev = NULL;
    node->next = NULL;

    lp->capacity -= 1;

    free(node);
}






