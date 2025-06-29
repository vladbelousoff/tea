%include {
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <assert.h>

    // Forward declarations
    typedef enum {
        NODE_PROGRAM,
        NODE_FUNCTION,
        NODE_PARAM,
        NODE_ATTR
    } NodeType;

    typedef struct ASTNode {
        NodeType type;
        char *value;
        struct ASTNode **children;
        int child_count;
        int line;
        int column;
    } ASTNode;

    // External function declarations (implemented in ast.c)
    extern ASTNode* create_node(NodeType type, const char *value);
    extern void add_child(ASTNode *parent, ASTNode *child);
}

%token_type {ASTNode*}
%default_type {ASTNode*}
%extra_argument {ASTNode **result}

// Token definitions - only what we need
%token_prefix TEA_TOKEN_
%token FN IDENT LPAREN RPAREN LBRACE RBRACE.
%token AT COLON COMMA.

// Grammar rules - simplified for functions with empty bodies only
program(A) ::= item_list(B). {
    A = create_node(NODE_PROGRAM, NULL);
    if (B && B->children && B->child_count > 0) {
        for (int i = 0; i < B->child_count; i++) {
            if (B->children[i]) {
                add_child(A, B->children[i]);
            }
        }
    }
    *result = A;
    printf("Parsed TEA program successfully!\n");
}

item_list(A) ::= item_list(B) item(C). {
    A = B ? B : create_node(NODE_PROGRAM, NULL);
    if (C) add_child(A, C);
}
item_list(A) ::= item(B). {
    A = create_node(NODE_PROGRAM, NULL);
    if (B) add_child(A, B);
}

item(A) ::= attr_list(B) function(C). {
    A = C;
    if (B && B->children && B->child_count > 0) {
        for (int i = 0; i < B->child_count; i++) {
            if (B->children[i]) {
                add_child(A, B->children[i]);
            }
        }
    }
}
item(A) ::= function(B). { A = B; }

attr_list(A) ::= attr_list(B) attribute(C). {
    A = B ? B : create_node(NODE_ATTR, NULL);
    if (C) add_child(A, C);
}
attr_list(A) ::= attribute(B). {
    A = create_node(NODE_ATTR, NULL);
    if (B) add_child(A, B);
}

attribute(A) ::= AT IDENT(B). {
    A = create_node(NODE_ATTR, B ? B->value : NULL);
}

// Function with empty body only
function(A) ::= FN IDENT(B) LPAREN param_list(C) RPAREN LBRACE RBRACE. {
    A = create_node(NODE_FUNCTION, B ? B->value : NULL);
    if (C) add_child(A, C);
}

param_list(A) ::= param_list(B) COMMA parameter(C). {
    A = B ? B : create_node(NODE_PARAM, NULL);
    if (C) add_child(A, C);
}
param_list(A) ::= parameter(B). {
    A = create_node(NODE_PARAM, NULL);
    if (B) add_child(A, B);
}
param_list(A) ::= . { A = NULL; }

parameter(A) ::= IDENT(B) COLON IDENT(C). {
    A = create_node(NODE_PARAM, B ? B->value : NULL);
    add_child(A, create_node(NODE_PARAM, C ? C->value : NULL));
}

%syntax_error {
    fprintf(stderr, "Syntax error in parser\n");
}