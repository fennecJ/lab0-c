#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/**
 * mergeTwoLists() - merge two SINGULAR SORTED list into one SINGULAR SORTED
 * list in lexical order by element_t structure's value
 * @l1: header of first singular sorted list
 * @l2: header of second singular sorted list
 * @descend: whether to merge lists sorted in descending order
 *
 * Return: merged list
 */
struct list_head *mergeTwoLists(struct list_head *l1,
                                struct list_head *l2,
                                bool descend);

/**
 * @brief rebuild_list_link() - rebuild given singular list into
 * doubly list. Mind one should always ensure the list is singular
 * before call to this function.
 *
 * @param head header of the list
 */
static inline void rebuild_list_link(struct list_head *head);

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *li = malloc(sizeof(struct list_head));
    if (li)
        INIT_LIST_HEAD(li);
    return li;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;
    element_t *entry;
    element_t *saf;
    list_for_each_entry_safe (entry, saf, head, list) {
        if (entry) {
            if (entry->value)
                free(entry->value);
            free(entry);
        }
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *ele = malloc(sizeof(element_t));
    if (!ele)
        return false;
    ele->value = strdup(s);

    // If mem allocate for str fail
    if (!ele->value) {
        free(ele);
        return false;
    }
    list_add(&ele->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *ele = malloc(sizeof(element_t));
    if (!ele)
        return false;
    ele->value = strdup(s);

    // If mem allocate for str fail
    if (!ele->value) {
        free(ele);
        return false;
    }
    list_add_tail(&ele->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    // mind that first head is just begin of circular list and doesn't embedded
    // in element(1st entry is located in 2nd node)
    struct list_head *true_begin = head->next;
    element_t *ele = list_entry(true_begin, element_t, list);
    list_del(true_begin);
    if (!ele)
        return ele;
    if (sp && ele->value) {
        strncpy(sp, ele->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return ele;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    struct list_head *true_begin = head->prev;
    element_t *ele = list_entry(true_begin, element_t, list);
    list_del(true_begin);
    if (!ele)
        return ele;
    if (sp && ele->value) {
        strncpy(sp, ele->value, bufsize - 1);
        sp[bufsize - 1] = '\0';
    }
    return ele;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;

    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head)
        return false;
    // One move forward, one move backward, they'll meet in the middle in
    // circular linked-list
    struct list_head *forward;
    struct list_head *backward;
    forward = head;
    backward = head->prev;

    while (forward != backward && forward->next != backward) {
        forward = forward->next;
        backward = backward->prev;
    }
    element_t *target = list_entry(backward, element_t, list);
    list_del(backward);
    if (!target || !target->value) {
        return false;
    } else {
        free(target->value);
        free(target);
        return true;
    }
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *n;
    struct list_head *sf;
    struct list_head *tmp;
    list_for_each_safe (n, sf, head) {
        tmp = n->next;
        n->next = n->prev;
        n->prev = tmp;
    }
    // list_for_each start on head->next, we need to modify head itself
    // likewisely
    tmp = head->next;
    head->next = head->prev;
    head->prev = tmp;
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
}

static struct list_head *q_merge_sort(struct list_head *head, bool descend)
{
    if (!head || !head->next)
        return head;

    struct list_head *slow = head;
    for (struct list_head *fast = head->next; fast && fast->next;
         fast = fast->next->next)
        slow = slow->next;
    struct list_head *mid = slow->next;
    slow->next = NULL;

    struct list_head *left = q_merge_sort(head, descend),
                     *right = q_merge_sort(mid, descend);
    return mergeTwoLists(left, right, descend);
}


/* Rebuild singular list back to doubly list again */
static inline void rebuild_list_link(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *node, *prev;
    prev = head;
    node = head->next;
    while (node) {
        node->prev = prev;
        prev = node;
        node = node->next;
    }
    prev->next = head;
    head->prev = prev;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    head->prev->next = NULL;
    head->next = q_merge_sort(head->next, descend);
    rebuild_list_link(head);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return 0;
}

/**
 * mergeTwoLists() - merge two SINGULAR SORTED list into one SINGULAR SORTED
 * list in lexical order by element_t structure's value
 * @l1: header of first singular sorted list
 * @l2: header of second singular sorted list
 * @descend: whether to merge lists sorted in descending order
 *
 * Return: merged list
 */
struct list_head *mergeTwoLists(struct list_head *l1,
                                struct list_head *l2,
                                bool descend)
{
    struct list_head *head = NULL, **ptr = &head, **node;
    for (node = NULL; l1 && l2; *node = (*node)->next) {
        element_t *ele1 = list_entry(l1, element_t, list);
        element_t *ele2 = list_entry(l2, element_t, list);

        node = ((strcmp(ele1->value, ele2->value) < 0) ^ descend) ? &l1 : &l2;
        *ptr = *node;
        ptr = &(*ptr)->next;
    }
    *ptr = (struct list_head *) ((uintptr_t) l1 | (uintptr_t) l2);
    return head;
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head || list_empty(head))
        return 0;
    else if (list_is_singular(head))
        return list_entry(head->next, queue_contex_t, chain)->size;

    queue_contex_t *qct = list_entry(head->next, queue_contex_t, chain);
    queue_contex_t *entry;
    queue_contex_t *saf;

    int ele_cnt = qct->size;
    qct->q->prev->next = NULL;  // make list singular

    list_for_each_entry_safe (entry, saf, head, chain) {
        if (!entry || entry == qct)
            continue;
        ele_cnt += entry->size;
        entry->q->prev->next = NULL;  // make list singular
        qct->q->next = mergeTwoLists(qct->q->next, entry->q->next, descend);
        entry->size = 0;
        entry->q->next = entry->q;
    }
    // rebuild prev link;
    rebuild_list_link(qct->q);
    return ele_cnt;
}
