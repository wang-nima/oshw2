#include "cs402.h"
#include "my402list.h"
#include <stdlib.h>

#define Anchor_Ptr &(l -> anchor)

int  My402ListLength(My402List* l)
{
	return l -> num_members;
}

int  My402ListEmpty(My402List* l)
{
	return l -> num_members == 0 ? TRUE: FALSE;
}

int  My402ListAppend(My402List* l, void* object)
{
	My402ListElem* x = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(x == NULL)
		return FALSE;
	x -> obj = object;
	if(My402ListEmpty(l) == TRUE)
	{
		x -> prev = &(l -> anchor);
		x -> next = &(l -> anchor);
		l -> anchor.next = x;
		l -> anchor.prev = x;
	}
	else
	{
		x -> prev = My402ListLast(l);
		x -> next = &(l -> anchor);
		x -> prev -> next = x;
		l -> anchor.prev = x;
	}
	(l -> num_members)++;
	return TRUE;
}


int  My402ListPrepend(My402List* l, void* object)
{
	My402ListElem* x = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(x == NULL)
		return FALSE;
	x -> obj = object;
	if(My402ListEmpty(l) == TRUE)
	{
		x -> prev = &(l -> anchor);
		x -> next = &(l -> anchor);
		l -> anchor.next = x;
		l -> anchor.prev = x;
	}
	else
	{
		x -> next = My402ListFirst(l);
		x -> prev = &(l -> anchor);
		l -> anchor.next = x;
		x -> next -> prev = x;
	}
	(l -> num_members)++;
	return TRUE;
}
void My402ListUnlink(My402List* l, My402ListElem* elem)
{
	if(elem == My402ListFirst(l))
	{
		l -> anchor.next = elem -> next;
		l -> anchor.next -> prev = &(l -> anchor);
	}
	else if(elem == My402ListLast(l))
	{
		l -> anchor.prev = elem -> prev;
		l -> anchor.prev -> next = &(l -> anchor);
	}
	else
	{
		elem -> next -> prev = elem -> prev;
		elem -> prev -> next = elem -> next;
	}
	free(elem);
	(l -> num_members)--;
}
void My402ListUnlinkAll(My402List* l)
{
	My402ListElem* p,*q;
	p = (l -> anchor).next;
	while(p != &(l -> anchor) )
	{
		q = p -> next;
		free(p);
		p = q;
	}
	(l -> num_members) = 0;
}

int  My402ListInsertAfter(My402List* l, void* object, My402ListElem* elme)
{
	My402ListElem* x = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(x == NULL)
		return FALSE;
	x -> obj = object;

	x -> next = elme -> next;
	x -> prev = elme;
	elme -> next = x;
	x -> next -> prev = x;
	
	(l -> num_members)++;
	return TRUE;
}

int  My402ListInsertBefore(My402List* l, void* object, My402ListElem* elem)
{
	My402ListElem* x = (My402ListElem*)malloc(sizeof(My402ListElem));
	if(x == NULL)
		return FALSE;
	x -> obj = object;

	x -> next = elem;
	x -> prev = elem -> prev;
	elem -> prev = x;
	x -> prev -> next = x;

	(l -> num_members)++;
	return TRUE;
}

My402ListElem *My402ListFirst(My402List* l)
{
	return l -> anchor.next;
}

My402ListElem *My402ListLast(My402List* l)
{
	return l -> anchor.prev;
}

My402ListElem *My402ListNext(My402List* l, My402ListElem* elem)
{
	return elem -> next == &(l -> anchor) ? NULL : elem -> next;
}

My402ListElem *My402ListPrev(My402List* l, My402ListElem* elem)
{
	return elem -> prev == &(l -> anchor) ? NULL : elem -> prev;
}

My402ListElem *My402ListFind(My402List* l, void* object)
{
	My402ListElem* it;
	it = My402ListFirst(l);
	while(it != Anchor_Ptr)
	{
		if(it -> obj == object)
			return it;
		it = it -> next;
	}
	return NULL;
}

int My402ListInit(My402List* l)
{
	l -> num_members = 0;
	l -> anchor.obj = NULL;
	l -> anchor.next = &(l -> anchor);
	l -> anchor.prev = &(l -> anchor);
	return TRUE;
}
