#pragma once

#include <misc/list.h>

typedef struct TREE_NODE
{
    void *value;
    list_t *children;
    struct TREE_NODE *parent;
} TreeNode_t;

typedef struct TREE
{
    size_t nodes;
    TreeNode_t *root;
} Tree_t;

Tree_t *tree_create();
void tree_set_root(Tree_t *tree, void *value);
TreeNode_t *tree_create_node(void *value);
void tree_insert(Tree_t *tree, TreeNode_t *parent, void *value);
void tree_remove(Tree_t *tree, TreeNode_t *node);