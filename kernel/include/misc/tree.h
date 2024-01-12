#pragma once

#include <misc/list.h>
#include <arch/lock.h>

typedef struct TREE_NODE
{
    void *value;
    list_t *children;
    struct TREE_NODE *parent;
} TreeNode_t;

typedef struct TREE
{
    TreeNode_t *root;
    size_t nodes;
    lock_t lock;
} Tree_t;

Tree_t *tree_create();
void tree_set_root(Tree_t *tree, void *value);
TreeNode_t *tree_create_node(void *value);
void tree_insert(Tree_t *tree, TreeNode_t *parent, TreeNode_t *node);
void tree_remove(Tree_t *tree, TreeNode_t *node);