#include "tea_lang.h"

#include <rtl_log.h>

// Create a new AST node
ASTNode *create_node(NodeType type, const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        rtl_log_err(, "Error: Failed to allocate memory for AST node\n");
        return NULL;
    }

    node->type = type;
    node->value = value ? _strdup(value) : NULL;
    node->children = NULL;
    node->child_count = 0;
    node->line = 0;
    node->column = 0;

    return node;
}

// Add a child node to a parent node
void add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) {
        return;
    }

    parent->children = realloc(parent->children, sizeof(ASTNode *) * (parent->child_count + 1));
    if (!parent->children) {
        rtl_log_err(, "Error: Failed to allocate memory for child nodes\n");
        return;
    }

    parent->children[parent->child_count++] = child;
}

// Free an AST node and all its children
void free_ast(ASTNode *node) {
    if (!node) {
        return;
    }

    // Free all children first
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }

    // Free the children array
    if (node->children) {
        free(node->children);
    }

    // Free the value string
    if (node->value) {
        free(node->value);
    }

    // Free the node itself
    free(node);
}

// Get the name of a node type for printing
static const char *get_node_type_name(NodeType type) {
    switch (type) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_FUNCTION: return "FUNCTION";
        case NODE_PARAM: return "PARAM";
        case NODE_ATTR: return "ATTR";
        default: return "UNKNOWN";
    }
}

// Print an AST node and its children (recursive)
void print_ast(ASTNode *node, int depth) {
    if (!node) {
        return;
    }

    // Print indentation
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }

    // Print node type and value
    printf("%s", get_node_type_name(node->type));
    if (node->value) {
        printf(": %s", node->value);
    }

    // Print location info if available
    if (node->line > 0 && node->column > 0) {
        printf(" (line %d, col %d)", node->line, node->column);
    }

    printf("\n");

    // Print all children
    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], depth + 1);
    }
}
