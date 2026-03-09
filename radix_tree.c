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
    bool has_children;
    union radix_slot slot; 
};

#define BITS_IN_PTR ((sizeof(void*)) * 8)
const int max_level = BITS_IN_PTR / TREE_WIDTH;

struct range* _get_entry_range(void *entry) {
    return (struct range*) entry;
}

bool ranges_overlap(struct range *a, struct range *b) {
    return (a->left > b->right) || (b->left > a->right);
}

inline int _get_idx_in_node(int entry_pos, int level, int parent_bits) {
    int entry_parent_bits = entry_pos >> ((max_level - level) * TREE_WIDTH);
    bool entry_in_node = (entry_parent_bits == parent_bits);
    if (!entry_in_node) {
        return (entry_parent_bits < parent_bits)? 0 : (1 << TREE_WIDTH) - 1;
    }
    return (uintptr_t) entry_pos >> ((max_level - 1 - level) * TREE_WIDTH);
}

inline struct range _get_idx_range(struct range range, int level, int parent_idx_bits) {
    int left = _get_idx_in_node(range.left, level, parent_idx_bits);
    int right = _get_idx_in_node(range.right, level, parent_idx_bits);
    struct range ans = {
        .left = left,
        .right = right,
    };
    return ans;
}

void* _is_empty_range(struct range range, struct radix_tree* cur, int level, uint parent_idx_bits) {
    if (cur == NULL) {
        return NULL;
    }
    if (!cur->has_children) {
        return cur->slot.data;
    }

    int left_idx = _get_idx_in_node(range.left, level, parent_idx_bits);
    int right_idx = _get_idx_in_node(range.right, level, parent_idx_bits);

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
    if (!cur->has_children) {
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
// TODO: need to have cleanup mechanism in case insertion fails halfway
int _insert_entry(void* entry, struct radix_tree* cur, int level, uint parent_idx_bits) {
    // Leaf Node
    if (!cur->has_children) {
        if (cur->slot.data== NULL) {
            cur->slot.data = entry;
            return 0;
        }

        void* existing_entry = cur->slot.data;
        struct range* existing_entry_range = _get_entry_range(existing_entry);
        struct range* new_entry_range = _get_entry_range(entry);

        bool overlap = ranges_overlap(existing_entry_range, new_entry_range);
        assert(!overlap);

        int array_size = (1 << (TREE_WIDTH));

        struct radix_tree* children_array = malloc(sizeof(*children_array) * array_size);

        if (children_array == NULL) {
            return -1;
        }

        cur->has_children = true;
        cur->slot.children = children_array;
        memset(children_array, 0, array_size);

        _insert_entry(existing_entry, cur, level, parent_idx_bits);
        return _insert_entry(entry, cur, level, parent_idx_bits);
    }

    struct range* entry_range = _get_entry_range(entry);
    struct range idx_range = _get_idx_range(*entry_range, level, parent_idx_bits);
    struct radix_tree* child_arr = cur->slot.children;
    int succ = 0;
    for (int i = idx_range.left; i <= idx_range.right; i++) {
        int child_parent_bits = (parent_idx_bits << TREE_WIDTH) | i;
        succ |= _insert_entry(entry, &child_arr[i], level + 1, child_parent_bits);
    }

    return succ;
}

// Return 0 on success, -1 on error
int insert_entry(void* entry, struct radix_tree* root) {
    struct range* new_entry_range = _get_entry_range(entry);
    if (is_empty_range(*new_entry_range, root)) {
        return -1;
    }
    return _insert_entry(entry, root, 0, 0);
}
