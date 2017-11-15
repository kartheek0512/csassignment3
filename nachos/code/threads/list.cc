// list.cc
//
//     	Routines to manage a singly-linked list of "things".
//
// 	A "ListElement" is allocated for each item to be put on the
//	list; it is de-allocated when the item is removed. This means
//      we don't need to keep a "next" pointer in every object we
//      want to put on a list.
//
//     	NOTE: Mutual exclusion must be provided by the caller.
//  	If you want a synchronized list, you must use the routines
//	in synchlist.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "list.h"
#include "thread.h"

//Edited_Start
//---------------------------------------------
// If any one the key matches with the given key then  this function
// return item of that element
//----------------------------------------------

void *
List::Search(int key)
{
   ListElement * temp = this->first;
   while(temp!=NULL){
   	if(temp->key == key)return temp->item;
	temp = temp->next;
	}
   return NULL;
}

//--------------------------------------------------------------
// Searches for which element contains the given key and removes it from the list.
// Return value will be 0 if the remove is successfull and it'll be 1 if there's no such element in the list
//--------------------------------------------------------------

int
List::SearchAndRemove(int key)
{
   ListElement * prev = NULL;
   ListElement * curr = first;
   while(curr!=NULL){
	if(curr->key == key){
		if(prev==NULL)this->first = curr->next;
		else prev->next = curr->next;
		delete curr;
		return 0;
		}
	else{
		prev = curr;
		curr = curr->next;
		}
	}
    return -1;
}
//Edited_Stop


//----------------------------------------------------------------------
// ListElement::ListElement
// 	Initialize a list element, so it can be added somewhere on a list.
//
//	"itemPtr" is the item to be put on the list.  It can be a pointer
//		to anything.
//	"sortKey" is the priority of the item, if any.
//----------------------------------------------------------------------

ListElement::ListElement(void *itemPtr, int sortKey)
{
     item = itemPtr;
     key = sortKey;
     next = NULL;	// assume we'll put it at the end of the list
}

//----------------------------------------------------------------------
// List::List
//	Initialize a list, empty to start with.
//	Elements can now be added to the list.
//----------------------------------------------------------------------

List::List()
{
    first = last = NULL;
}

//----------------------------------------------------------------------
// List::~List
//	Prepare a list for deallocation.  If the list still contains any
//	ListElements, de-allocate them.  However, note that we do *not*
//	de-allocate the "items" on the list -- this module allocates
//	and de-allocates the ListElements to keep track of each item,
//	but a given item may be on multiple lists, so we can't
//	de-allocate them here.
//----------------------------------------------------------------------

List::~List()
{
    while (Remove() != NULL)
	;	 // delete all the list elements
}

//----------------------------------------------------------------------
// List::Append
//      Append an "item" to the end of the list.
//
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the end.
//
//	"item" is the thing to put on the list, it can be a pointer to
//		anything.
//----------------------------------------------------------------------

void
List::Append(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it after last
	last->next = element;
	last = element;
    }
}

//----------------------------------------------------------------------
// List::Prepend
//      Put an "item" on the front of the list.
//
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, put it at the beginning.
//
//	"item" is the thing to put on the list, it can be a pointer to
//		anything.
//----------------------------------------------------------------------

void
List::Prepend(void *item)
{
    ListElement *element = new ListElement(item, 0);

    if (IsEmpty()) {		// list is empty
	first = element;
	last = element;
    } else {			// else put it before first
	element->next = first;
	first = element;
    }
}

//----------------------------------------------------------------------
// List::Remove
//      Remove the first "item" from the front of the list.
//
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//----------------------------------------------------------------------

void *
List::Remove()
{
    return SortedRemove(NULL);  // Same as SortedRemove, but ignore the key
}

//----------------------------------------------------------------------
// List::Mapcar
//	Apply a function to each item on the list, by walking through
//	the list, one element at a time.
//
//	Unlike LISP, this mapcar does not return anything!
//
//	"func" is the procedure to apply to each element of the list.
//----------------------------------------------------------------------

void
List::Mapcar(VoidFunctionPtr func)
{
    for (ListElement *ptr = first; ptr != NULL; ptr = ptr->next) {
       DEBUG('l', "In mapcar, about to invoke %x(%x)\n", func, ptr->item);
       (*func)((int)ptr->item);
    }
}

//----------------------------------------------------------------------
// List::IsEmpty
//      Returns TRUE if the list is empty (has no items).
//----------------------------------------------------------------------

bool
List::IsEmpty()
{
    if (first == NULL)
        return TRUE;
    else
	return FALSE;
}

//----------------------------------------------------------------------
// List::SortedInsert
//      Insert an "item" into a list, so that the list elements are
//	sorted in increasing order by "sortKey".
//
//	Allocate a ListElement to keep track of the item.
//      If the list is empty, then this will be the only element.
//	Otherwise, walk through the list, one element at a time,
//	to find where the new item should be placed.
//
//	"item" is the thing to put on the list, it can be a pointer to
//		anything.
//	"sortKey" is the priority of the item.
//----------------------------------------------------------------------

void
List::SortedInsert(void *item, int sortKey)
{
    ListElement *element = new ListElement(item, sortKey);
    ListElement *ptr;		// keep track

    if (IsEmpty()) {	// if list is empty, put
        first = element;
        last = element;
    } else if (sortKey < first->key) {
		// item goes on front of list
	element->next = first;
	first = element;
    } else {		// look for first elt in list bigger than item
        for (ptr = first; ptr->next != NULL; ptr = ptr->next) {
            if (sortKey < ptr->next->key) {
		element->next = ptr->next;
	        ptr->next = element;
		return;
	    }
	}
	last->next = element;		// item goes at end of list
	last = element;
    }
}

//----------------------------------------------------------------------
// List::SortedRemove
//      Remove the first "item" from the front of a sorted list.
//
// Returns:
//	Pointer to removed item, NULL if nothing on the list.
//	Sets *keyPtr to the priority value of the removed item
//	(this is needed by interrupt.cc, for instance).
//
//	"keyPtr" is a pointer to the location in which to store the
//		priority of the removed item.
//----------------------------------------------------------------------

void *
List::SortedRemove(int *keyPtr)
{
    ListElement *element = first;
    void *thing;

    if (IsEmpty())
	return NULL;

    thing = first->item;
    if (first == last) {	// list had one item, now has none
        first = NULL;
	last = NULL;
    } else {
        first = element->next;
    }
    if (keyPtr != NULL)
        *keyPtr = element->key;
    delete element;
    return thing;
}

void*
List::GetMinPriorityThread (void)
{
   ListElement *ptr=first, *prev=NULL, *minptr=ptr, *minprev=prev;
   void *thing;
   int minimum;

    if (IsEmpty())
        return NULL;

    if (first == last) {        // list had one item, now has none
        thing = first->item;
        first = NULL;
        last = NULL;
    } else {
        minimum = ((NachOSThread*)(minptr->item))->GetPriority();
        for (ptr = first->next, prev = first; ptr != NULL; prev = ptr, ptr = ptr->next) {
           if (((NachOSThread*)(ptr->item))->GetPriority() < minimum) {
              minptr = ptr;
              minprev = prev;
              minimum = ((NachOSThread*)(minptr->item))->GetPriority();
           }
        }
        ASSERT(minptr != NULL);
        ASSERT(minimum == ((NachOSThread*)(minptr->item))->GetPriority());
        thing = minptr->item;
        if (minprev == NULL) { // First element has minimum priority
           first = first->next;
        }
        else if (minptr->next == NULL) { // Last element has minimum priority
           ASSERT(minprev->next == minptr);
           minprev->next = NULL;
           last = minprev;
        }
        else {
           ASSERT(minprev->next == minptr);
           minprev->next = minptr->next;
        }
    }
    delete minptr;
    return thing;
}
