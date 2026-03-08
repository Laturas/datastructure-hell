#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define TREE_WIDTH 8
#define TREE_MASK ((1u << TREE_WIDTH) - 1u)


struct range {
    int left;
    int right;
};

union radix_slot {
    // NOTE: The data struct must have a range struct embedded as the first field in it
    void* data; 
    struct radix_tree* children;
};

struct radix_tree {
    uint child_cnt;
    union radix_slot slot; 
};

#define BITS_IN_PTR ((sizeof(void*)) * 8)
const int max_level = BITS_IN_PTR / TREE_WIDTH;

struct range* entry_range(void *entry) {
    return (struct range*) entry;
}

bool ranges_overlap(struct range *a, struct range *b) {
    return (a->left > b->right) || (b->left > a->right);
}

void* _is_empty_range(struct range range, struct radix_tree* cur, int level, uint parent_idx_bits) {
    if (cur == NULL) {
        return NULL;
    }
    if (cur->child_cnt != 0) {
        return cur->slot.data;
    }

    int left_idx = 0;
    int right_idx = (1 << TREE_WIDTH) - 1;
    bool left_bound_in_node = range.left >> (level * TREE_WIDTH) == parent_idx_bits;
    bool right_bound_in_node = range.right >> (level * TREE_WIDTH) == parent_idx_bits;
    if (left_bound_in_node) {
        left_idx = (uintptr_t)range.left >> ((max_level - 1 - level) * TREE_WIDTH);
        left_idx &= TREE_MASK;
    }
    if (right_bound_in_node) {
        right_idx = (uintptr_t)range.right >> ((max_level - 1 - level) * TREE_WIDTH);
        right_idx &= TREE_MASK;
    }

    uintptr_t ans;
    struct radix_tree* child_array = cur->slot.children;
    for (int i = left_idx; i <= right_idx; i++) {
        uint child_idx_bits = (parent_idx_bits << TREE_WIDTH) | i;
        ans |= (uintptr_t) _is_empty_range(range, &child_array[i], level + 1, child_idx_bits);
    }

    return (void*) ans;
}

// Return Null if empty, nonsense otherwise
void* is_empty_range(struct range range, struct radix_tree* root) {
    return _is_empty_range(range, root, 0, 0);
}

// Returns the data associated with the given address, or NULL if not found.
void* _lookup_entry(void* addr, struct radix_tree* cur, int level) {
    // base case
    if (cur == NULL) {
        return NULL;
    }
    if (cur->child_cnt != 0) {
        return cur->slot.data;
    }

    // get the index of the next node
    int idx = (uintptr_t)addr >> ((max_level - 1 - level) * TREE_WIDTH);
    idx &= TREE_MASK;

    // recursively lookup the next node
    struct radix_tree* next = &cur->slot.children[idx];
    return _lookup_entry(addr, next, level + 1);
}

// Returns entry that contains the address
void* lookup_entry(void* addr, struct radix_tree* root) {
    return _lookup_entry(addr, root, 0);
}

// Returns 0 on success, -1 on error
// TODO: return MEMERR on malloc failure to distinguish between causes of failure
int _insert_entry(void* entry, struct radix_tree* cur, int level) {
    // Leaf Node
    if (cur->child_cnt != 0) {
        if (cur->slot.data== NULL) {
            cur->slot.data = entry;
            return 0;
        }

        void* existing_entry = cur->slot.data;
        struct range* existing_entry_range = entry_range(existing_entry);
        struct range* new_entry_range = entry_range(entry);

        bool overlap = ranges_overlap(existing_entry_range, new_entry_range);
        assert(!overlap);

        int array_size = (1 << (TREE_WIDTH));

        struct radix_tree* children_array = malloc(sizeof(*children_array) * array_size);

        if (children_array == NULL) {
            return -1;
        }

        cur->child_cnt++;
        cur->slot.children = children_array;
        memset(children_array, 0, array_size);

        // _insert_entry(existing_entry, children_array, level);
        return _insert_entry(existing_entry, children_array, level);
    }

    return NULL;
}

int insert_entry(void* entry, struct radix_tree* root) {
    struct range* new_entry_range = entry_range(entry);
    if (is_empty_range(*new_entry_range, root)) {
        return -1;
    }
    return _insert_entry(entry, root, 0);
}
