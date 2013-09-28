#ifndef MISC_LIST_H
#define MISC_LIST_H

#include <cassert>

namespace Misc{

/*
  A simple and completely allocation-less doubly linked list. The
  class only manages pointers to and between elements. It leaving all
  memory management to the user.
*/
template <typename Elem>
struct List
{
  List() : head(0), tail(0), totalNum(0) {}

  // Empty the list.
  void reset()
  {
    head = 0;
    tail = 0;
    totalNum = 0;
  }

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

  // Swap the contents of this list with another of the same type
  void swap(List &other)
  {
    Elem *tmp;

    tmp = head;
    head = other.head;
    other.head = tmp;

    tmp = tail;
    tail = other.tail;
    other.tail = tmp;

    unsigned int tmp2 = totalNum;
    totalNum = other.totalNum;
    other.totalNum = tmp2;
  }

  /* Absorb the contents of another list. All the elements from the
     list are moved to the end of this list, and the other list is
     cleared.
   */
  void absorb(List &other)
  {
    assert(&other != this);
    if(other.totalNum)
      {
        absorb(other.head, other.tail, other.totalNum);
        other.reset();
      }
    assert(other.totalNum == 0);
  }

  /* Absorb a range of elements, endpoints included. The elements are
     assumed NOT to belong to any list, but they ARE assumed to be
     connected with a chain between them.

     The connection MUST run all the way from 'first' to 'last'
     through the ->next pointers, and vice versa through ->prev
     pointers.

     The parameter 'num' must give the exact number of elements in the
     chain.

     Passing first == last, num == 1 is allowed and is equivalent to
     calling insert().
  */
  void absorb(Elem* first, Elem *last, int num)
  {
    assert(first && last && num>=1);
    if(tail)
      {
        // There are existing elements. Insert the first node at the
        // end of the list.
        assert(head && totalNum > 0);
        tail->next = first;
      }
    else
      {
        // This is the first element
        assert(head == 0 && totalNum == 0);
        head = first;
      }

    // These have to be done in either case
    first->prev = tail;
    last->next = 0;
    tail = last;

    totalNum += num;
  }

  Elem* getHead() const { return head; }
  Elem* getTail() const { return tail; }
  unsigned int getNum() const { return totalNum; }

private:

  Elem *head;
  Elem *tail;
  unsigned int totalNum;
};

}
#endif
