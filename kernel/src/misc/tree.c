#include <misc/tree.h>
#include <mem/heap.h>

Tree_t *tree_create()
{
    Tree_t *tree = (Tree_t *)kmalloc(sizeof(Tree_t));
    if (!tree)
        return NULL;

    tree->root = NULL;
    tree->nodes = 0;
    tree->lock = 0;

    return tree;
}

void tree_set_root(Tree_t *tree, void *value)
{
    tree->root = tree_create_node(value);
    tree->nodes = 1;
}

TreeNode_t *tree_create_node(void *value)
{
    TreeNode_t *node = (TreeNode_t *)kmalloc(sizeof(TreeNode_t));
    node->value = value;
    node->children = list_create();
    node->parent = NULL;
    if (!node->children)
    {
        kfree(node);
        return NULL;
    }

    return node;
}

void tree_insert(Tree_t *tree, TreeNode_t *parent, void *value)
{
    TreeNode_t *new = tree_create_node(value);
    if (!new)
        return;

    lock_acquire(&tree->lock);

    list_insert(parent->children, new);
    new->parent = parent;
    tree->nodes++;

    lock_release(&tree->lock);
}

void tree_remove(Tree_t *tree, TreeNode_t *node)
{
    TreeNode_t *parent = node->parent;
    tree->nodes--;

    lock_acquire(&tree->lock);
    
    list_delete(parent->children, list_find(parent->children, node));
    foreach(child, node->children)
    {
        ((TreeNode_t *)child->value)->parent = parent;
    }
    
    list_merge(parent->children, node->children);
    kfree(node);
    
    lock_release(&tree->lock);
}