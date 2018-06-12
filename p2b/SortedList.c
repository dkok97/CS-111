/*
 NAME: Dinkar Khattar
 EMAIL: dinkarkhattar@ucla.edu
 ID: 204818138
 */
 
#include "SortedList.h"
#include <sched.h>
#include <string.h>
#include <stdio.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    SortedListElement_t* add_before = list->next;
    while (add_before->key!=NULL && (strcmp(add_before->key, element->key) < 0))
    {
        add_before = add_before->next;
    }

    if(opt_yield & INSERT_YIELD)
        sched_yield();

    add_before->prev->next = element;
    element->prev = add_before->prev;
    element->next = add_before;
    add_before->prev = element;
}

int SortedList_delete( SortedListElement_t *element)
{
    if (element == NULL)
    {
        return 1;
    }

    if(opt_yield & DELETE_YIELD)
        sched_yield();

    SortedListElement_t* prev = element->prev;
    SortedListElement_t* next = element->next;

    if (prev->next!=next->prev)
    {
        return 1;
    }

    prev->next = element->next;
    next->prev = element->prev;

    return 0;
}


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    if (list == NULL || key == NULL)
    {
        return NULL;
    }

    SortedListElement_t *curr = list->next;

    if(opt_yield & LOOKUP_YIELD)
        sched_yield();

    while (curr->key!=NULL)
    {
        if ((strcmp(curr->key, key) == 0))
            return curr;
        else if (strcmp(curr->key, key) > 0)
            return NULL;
        curr = curr->next;
    }

    return NULL;
}

int SortedList_length(SortedList_t *list)
{
    if (list==NULL)
    {
        return 0;
    }

    if(opt_yield & LOOKUP_YIELD)
        sched_yield();

    int len = 0;

    SortedListElement_t *curr = list->next;

    while(curr->key!=NULL)
    {
        SortedListElement_t* prev = curr->prev;
        SortedListElement_t* next = curr->next;

        if (prev->next != curr || next->prev != curr)
        {
            return -1;
        }

        curr=curr->next;
        len++;
    }

    return len;
}
