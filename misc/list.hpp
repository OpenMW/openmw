#ifndef MISC_LIST_H
#define MISC_LIST_H

#include <assert.h>

namespace Misc{

/*
  This is just a suggested data structure for List. You can use
  anything that has next and prev pointers.
 */
template <typename X>
struct ListElem
{
  X data;
  ListElem *next;
  ListElem *prev;
};

/*
  A generic class that contains a doubly linked list of elements. It
  does not do any allocation of elements, it just keeps pointers to
  them.
*/
template <typename Elem>
struct List
{
  List() : head(0), tail(0), totalNum(0) {}

  // Insert an element at the end of the list. The element cannot be
  // part of any other list when this is called.
  void insert(Elem *p)
  {
    if(tail)
      {
        // There are existing elements. Insert the node at the end of
        // the list.
        assert(head && totalNum > 0);
        tail->next = p;
      }
    else
      {
        // This is the first element
        assert(head == 0 && totalNum == 0);
        head = p;
      }

    // These have to be done in either case
    p->prev = tail;
    p->next = 0;
    tail = p;

    totalNum++;
  }

  // Remove element from the list. The element MUST be part of the
  // list when this is called.
  void remove(Elem *p)
  {
    assert(totalNum > 0);

    if(p->next)
      {
        // There's an element following us. Set it up correctly.
        p->next->prev = p->prev;
        assert(tail && tail != p);
      }
    else
      {
        // We're the tail
        assert(tail == p);
        tail = p->prev;
      }

    // Now do exactly the same for the previous element
    if(p->prev)
      {
        p->prev->next = p->next;
        assert(head && head != p);
      }
    else
      {
        assert(head == p);
        head = p->next;
      }

    totalNum--;
  }

  // Pop the first element off the list
  Elem *pop()
  {
    Elem *res = getHead();
    if(res) remove(res);
    return res;
  }

  Elem* getHead() { return head; }
  Elem* getTail() { return tail; }
  unsigned int getNum() { return totalNum; }

private:

  Elem *head;
  Elem *tail;
  unsigned int totalNum;
};

}
#endif
