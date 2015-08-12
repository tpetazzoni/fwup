#ifndef CFGNODE_H
#define CFGNODE_H


/**
 * @brief key = value
 */
struct cfg_assignment
{
    char *key;
    struct cfg_node *value;
};

/**
 * @brief kind name { contents... }
 */
struct cfg_block
{
    // The kind of block (mbr, on-resource, etc.)
    char *kind;

    // An optional name for the block
    char *name;

    struct cfg_node *contents;
};

/**
 * @brief One item in a list
 */
struct cfg_element
{
    char *value;
};

/**
 * @brief name(elements...)
 */
struct cfg_function
{
    char *name;

    struct cfg_node *args;
};

enum cfg_node_type
{
    CFG_NODE_ASSIGNMENT,
    CFG_NODE_BLOCK,
    CFG_NODE_ELEMENT,
    CFG_NODE_FUNCTION,
    CFG_NODE_EMPTY // Placeholder when using NULL doesn't make sense
};

/**
 * @brief a node in the internal representation of the config
 */
struct cfg_node
{
    enum cfg_node_type type;
    struct cfg_node *next;

    union {
        struct cfg_assignment assignment;
        struct cfg_block block;
        struct cfg_element element;
        struct cfg_function fun;
    };
};


void print_cfg_node(const struct cfg_node *node);
void free_cfg_node(struct cfg_node *node);

struct cfg_node *create_cfg_empty();
struct cfg_node *create_cfg_assignment(char *key, struct cfg_node *value);
struct cfg_node *create_cfg_element(char *arg);
struct cfg_node *create_cfg_function(char *name, struct cfg_node *args);
struct cfg_node *create_cfg_block(char *kind, char *name, struct cfg_node *contents);

struct cfg_node *reverse_cfg_nodes(struct cfg_node *n);


#endif // CFGNODE_H

