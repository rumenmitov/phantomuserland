#ifndef __RADIX_TREE_H
#define __RADIX_TREE_H

#include "errno.h"
#include "ph_string.h"
#include <assert.h>

#define _RADIX_TREE_NODES 64

enum _RadixTreeNode {
  Node,
  Nil
};

struct RadixTree {
  enum _RadixTreeNode type;
  void *addr;
  struct RadixTree* children[_RADIX_TREE_NODES];
}; 


void radix_tree_init(struct RadixTree *radix, void *start, unsigned int number_of_addr) {
  assert(number_of_addr > 0);
  
  radix->type = Node;

  void* address_container[number_of_addr];
  struct RadixTree box[number_of_addr / _RADIX_TREE_NODES];

  for (int i = 0; i < number_of_addr / _RADIX_TREE_NODES; i++) {
    box[i].type = Node;
    ph_memcpy(box[i].children, address_container[i], _RADIX_TREE_NODES);
  }

}

errno_t radix_tree_insert(unsigned int) {}


#endif // __RADIX_TREE_H
