#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#define bool int
#define true 1
#define false 0

#ifndef NULL
    #define NULL (void*)0
#endif
#define TREE_WIDTH 8
#define TREE_MASK 0xff
struct prefix_tree;

enum tags {
  /* This bit indicates whether the data is loaded yet from the file */
  IS_LOADED = (1 << 0),
  /* This bit indicates whether the page has been swapped out. */
  IS_SWAPPED = (1 << 1),
  /* Indicates whether this page comes from an mmapped file */
  IS_MMAP_FILE = (1 << 2),
};


struct sup_pt_entry {
  void* vaddr;
  int run_length; // Number of continuous virtual pages taken up by this entry 
  enum tags sup_tags;
  struct frame* frame_ptr;
  struct file* from_file; // Can be NULL if it is not from a file.
};

union lookup_union {
    struct sup_pt_entry* pagetable;
    struct prefix_tree* children;
};

struct prefix_tree {
    bool has_children;
    union lookup_union children_or_pagetable;
};

#define BITS_IN_PTR ((sizeof(void*)) * 8) 
const int max_level = BITS_IN_PTR / TREE_WIDTH;

// 0 -> highest bits


struct sup_pt_entry* _lookup_entry(void* vaddr, struct prefix_tree cur, int level) {
    if (!cur.has_children) {
        return cur.children_or_pagetable.pagetable;
    }

    int ref_bits = (uintptr_t)vaddr >> ((max_level - 1 - level) * TREE_WIDTH);
    ref_bits &= TREE_MASK;

    struct prefix_tree next = cur.children_or_pagetable.children[ref_bits];
    return _lookup_entry(vaddr, next, level + 1);
}

struct sup_pt_entry* lookup_entry(void* vaddr, struct prefix_tree* root) {
    return _lookup_entry(vaddr, *root, 0);
}

bool _insert_spt_entry(struct sup_pt_entry* entry, struct prefix_tree* cur, int level) {
    if (!cur->has_children) {
        if (cur->children_or_pagetable.pagetable == NULL) {
            cur->children_or_pagetable.pagetable = entry;
            return true;
        }
        struct sup_pt_entry *existing_entry = cur->children_or_pagetable.pagetable;
        int array_size = (1 << (TREE_WIDTH));

        struct prefix_tree* children_array = malloc(sizeof(*children_array) * array_size);

        if (children_array == NULL) {
            return false;
        }


        cur->has_children = true;
        cur->children_or_pagetable.children = children_array;
        memset(children_array, 0, array_size);

        _insert_spt_entry(existing_entry, cur, level);
        return _insert_spt_entry(entry, cur, level);
    }

    void* vaddr = entry->vaddr;
    int ref_bits = (uintptr_t)vaddr >> ((max_level - 1 - level) * TREE_WIDTH);
    ref_bits &= TREE_MASK;

    struct prefix_tree* next = &cur->children_or_pagetable.children[ref_bits];
    return _insert_spt_entry(entry, next, level + 1);
}



bool insert_spt_entry(struct sup_pt_entry* entry, struct prefix_tree* root) {
    _insert_spt_entry(entry, root, 0);
}

void main(void) {

    
}
