#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "harness.h"

#ifndef strlcpy
#define strlcpy(dst, src, sz) snprintf((dst), (sz), "%s", (src))
#endif

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));
    if (!q) {
        printf("Error! Memory allocation failed\n");
        return q;
    }
    q->size = 0;
    q->tail = NULL;
    q->head = NULL;
    return q;
}


/* Free all storage used by queue */
void q_free(queue_t *q)
{
    if (q == NULL)
        return;
    while (q->head) {
        list_ele_t *t = q->head;
        q->head = q->head->next;
        free(t->value);
        free(t);
    }
    free(q);
}


/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
    list_ele_t *newh = malloc(sizeof(list_ele_t));
    if (!q || !newh) {
        free(newh);
        return false;
    }
    newh->value = malloc(sizeof(char) * strlen(s) + 1);
    if (!newh->value) {
        free(newh);
        return false;
    }
    strlcpy(newh->value, s, strlen(s) + 1);
    newh->next = q->head;
    q->head = newh;
    if (!q->tail)
        q->tail = newh;
    q->size++;
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
    list_ele_t *newh = malloc(sizeof(list_ele_t));
    if (!q || !newh) {
        free(newh);
        return false;
    }
    newh->value = malloc(sizeof(char) * (strlen(s) + 1));
    if (!newh->value) {
        free(newh);
        return false;
    }
    strlcpy(newh->value, s, strlen(s) + 1);
    newh->next = NULL;
    if (q->tail)
        q->tail->next = newh;
    else
        q->head = newh;
    q->tail = newh;
    q->size++;
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return true if successful.
 * Return false if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 * The space used by the list element and the string should be freed.
 */
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    if (!q)
        return false;
    if (!q->head)
        return false;
    strlcpy(sp, q->head->value, bufsize);
    list_ele_t *tmp = q->head;
    q->head = q->head->next;
    free(tmp->value);
    free(tmp);
    q->size--;
    return true;
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
    if (!q)
        return 0;
    return q->size;
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    if (!q)
        return;
    if (q->size < 2)
        return;
    list_ele_t *cursor = NULL;
    q->tail = q->head;
    while (q->head) {
        list_ele_t *next = q->head->next;
        q->head->next = cursor;
        cursor = q->head;
        q->head = next;
    }
    q->head = cursor;
}

list_ele_t *merge(list_ele_t *a, list_ele_t *b)
{
    list_ele_t *head = NULL;
    list_ele_t **tmp = &head;
    while (a && b) {
        if (strcmp(a->value, b->value) < 0) {
            *tmp = a;
            a = a->next;
        } else {
            *tmp = b;
            b = b->next;
        }
        tmp = &(*tmp)->next;
    }
    if (a)
        *tmp = a;
    if (b)
        *tmp = b;
    return head;
}

list_ele_t *split(list_ele_t *head)
{
    if (!head || !head->next)
        return head;
    list_ele_t *fast = head->next;
    list_ele_t *slow = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    fast = slow->next;
    slow->next = NULL;
    list_ele_t *a = split(head);
    list_ele_t *b = split(fast);
    return merge(a, b);
}


/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(queue_t *q)
{
    if (!q)
        return;
    if (q->size < 2)
        return;
    q->head = split(q->head);
    while (q->tail->next)
        q->tail = q->tail->next;
}
