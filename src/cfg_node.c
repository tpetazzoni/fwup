#include "cfg_node.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

struct cfg_node *create_cfg_element(char *value)
{
    struct cfg_node *n = (struct cfg_node *) malloc(sizeof(struct cfg_node));
    n->type = CFG_NODE_ELEMENT;
    n->next = NULL;
    n->element.value = value;
    return n;
}

struct cfg_node *create_cfg_empty()
{
    struct cfg_node *n = (struct cfg_node *) malloc(sizeof(struct cfg_node));
    n->type = CFG_NODE_EMPTY;
    n->next = NULL;
    return n;
}

struct cfg_node *create_cfg_function(char *name, struct cfg_node *args)
{
    struct cfg_node *n = (struct cfg_node *) malloc(sizeof(struct cfg_node));
    n->type = CFG_NODE_FUNCTION;
    n->next = NULL;
    n->fun.name = name;
    n->fun.args = args;
    return n;
}

struct cfg_node *create_cfg_assignment(char *key, struct cfg_node *value)
{
    struct cfg_node *n = (struct cfg_node *) malloc(sizeof(struct cfg_node));
    n->type = CFG_NODE_ASSIGNMENT;
    n->next = NULL;
    n->assignment.key = key;
    n->assignment.value = value;
    return n;
}

struct cfg_node *create_cfg_block(char *kind, char *name, struct cfg_node *contents)
{
    struct cfg_node *n = (struct cfg_node *) malloc(sizeof(struct cfg_node));
    n->type = CFG_NODE_BLOCK;
    n->next = NULL;
    n->block.kind = kind;
    n->block.name = name;
    n->block.contents = contents;
    return n;
}

struct cfg_node *reverse_cfg_nodes(struct cfg_node *n)
{
    struct cfg_node *h = NULL;

    while (n) {
        struct cfg_node *tmp = n;
        n = n->next;
        tmp->next = h;
        h = tmp;
    }

    return h;
}


#define ONE_KiB  (1024ULL)
#define ONE_MiB  (1024 * ONE_KiB)
#define ONE_GiB  (1024 * ONE_MiB)

#define NUM_ELEMENTS(X) (sizeof(X) / sizeof(X[0]))

struct suffix_multiplier
{
    const char *suffix;
    int64_t multiple;
};

static struct suffix_multiplier suffix_multipliers[] = {
    {"b", 512},
    {"kB", 1000},
    {"K", ONE_KiB},
    {"KiB", ONE_KiB},
    {"MB", 1000 * 1000},
    {"M", ONE_MiB},
    {"MiB", ONE_MiB},
    {"GB", 1000 * 1000 * 1000},
    {"G", ONE_GiB},
    {"GiB", ONE_GiB}
};

int64_t parse_int64(const char *str, bool *ok)
{
    char *suffix;

    errno = 0;
    int64_t value = strtoul(str, &suffix, 0);
    if (errno != 0 || suffix == str) {
        *ok = false;
        return 0;
    }

    if (*suffix == '\0') {
        *ok = true;
        return value;
    }

    int i;
    for (i = 0; i < NUM_ELEMENTS(suffix_multipliers); i++) {
        if (strcmp(suffix_multipliers[i].suffix, suffix) == 0) {
            *ok = true;
            return value * suffix_multipliers[i].multiple;
        }
    }

    //errx(EXIT_FAILURE, "Unknown size multiplier '%s'", suffix);
    *ok = false;
    return 0;
}

static void print_string(const char *str)
{
    bool ok;
    int64_t maybe_number = parse_int64(str, &ok);
    if (ok) {
        // Normalize numbers -> That way if we add suffixes or number formats in the future, old fwup versions will still work
        printf("%lld", (long long int) maybe_number);
    } else {
        // Always quote and escape
        printf("\"");
        for (; *str; str++) {
            switch (*str) {
            case '\n': printf("\\n"); break;
            case '\t': printf("\\t"); break;
            case '\r': printf("\\r"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\"': printf("\\\""); break;
            case '$':  printf("\\$"); break;

            default:
                if (isprint(*str))
                    printf("%c", *str);
                else
                    printf("\\%o", *str);
            }
        }
        printf("\"");
    }
}

static void print_list(const struct cfg_node *list)
{
    bool firstarg = true;
    for (; list && list->type == CFG_NODE_ELEMENT; list = list->next) {
        if (!firstarg)
            printf(",");
        firstarg = false;
        print_string(list->element.value);
    }
}

void print_cfg_node(const struct cfg_node *node)
{
    for (; node != NULL; node = node->next) {
        switch (node->type) {
        case CFG_NODE_ELEMENT:
            // Handled directly in function and assignment.
            break;

        case CFG_NODE_FUNCTION:
            printf("%s(", node->fun.name);
            print_list(node->fun.args);
            printf(")\n");
            break;

        case CFG_NODE_BLOCK:
            if (node->block.name)
                printf("%s %s {\n", node->block.kind, node->block.name);
            else
                printf("%s {\n", node->block.kind);
            print_cfg_node(node->block.contents);
            printf("}\n");
            break;

        case CFG_NODE_ASSIGNMENT:
            printf("%s=", node->assignment.key);
            if (node->assignment.value->next == NULL) {
                // One value
                print_string(node->assignment.value->element.value);
            } else {
                // Value is a list
                printf("{");
                print_list(node->assignment.value);
                printf("}");
            }
            printf("\n");
            break;

        case CFG_NODE_EMPTY:
            break;
        }
    }
}

/**
 * @brief Free all memory associated with the node and its children
 */
void free_cfg_node(struct cfg_node *node)
{
    while (node != NULL) {
        switch (node->type) {
        case CFG_NODE_ELEMENT:
            free(node->element.value);
            node->element.value = NULL;
            break;

        case CFG_NODE_FUNCTION:
            free(node->fun.name);
            free_cfg_node(node->fun.args);
            node->fun.name = NULL;
            node->fun.args = NULL;
            break;

        case CFG_NODE_BLOCK:
            free(node->block.kind);
            if (node->block.name)
                free(node->block.name);
            free_cfg_node(node->block.contents);
            node->block.kind = NULL;
            node->block.name = NULL;
            node->block.contents = NULL;
            break;

        case CFG_NODE_ASSIGNMENT:
            free(node->assignment.key);
            free_cfg_node(node->assignment.value);
            node->assignment.key = NULL;
            node->assignment.value = NULL;
            break;

        case CFG_NODE_EMPTY:
            break;
        }

        struct cfg_node *next = node->next;
        free(node);
        node = next;
    }
}
