%include
{
#include <stdlib.h>
#include <assert.h>
#include "tea_ast.h"
}

%token_type {tea_token_t*}
%default_type {tea_ast_node_t*}
%extra_argument {tea_ast_node_t **result}

%token_prefix TEA_TOKEN_
%token FN IDENT LPAREN RPAREN LBRACE RBRACE.
%token AT COLON COMMA.

program(A) ::= item_list(B). {
    A = tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (B) tea_ast_node_add_children(A, &B->children);
    *result = A;
}

item_list(A) ::= item_list(B) item(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

item_list(A) ::= item(B). {
    A = tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

item(A) ::= attr_list(B) function(C). {
    A = C;
    if (B) tea_ast_node_add_children(A, &B->children);
}

item(A) ::= function(B). { A = B; }

attr_list(A) ::= attr_list(B) attribute(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_ATTR, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

attr_list(A) ::= attribute(B). {
    A = tea_ast_node_create(TEA_AST_NODE_ATTR, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

attribute(A) ::= AT IDENT(B). {
    A = tea_ast_node_create(TEA_AST_NODE_ATTR, B);
}

function(A) ::= FN IDENT(B) LPAREN param_list(C) RPAREN LBRACE RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
}

param_list(A) ::= param_list(B) COMMA parameter(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

param_list(A) ::= parameter(B). {
    A = tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

param_list(A) ::= . { A = NULL; }

parameter(A) ::= IDENT(B) COLON IDENT(C). {
    A = tea_ast_node_create(TEA_AST_NODE_PARAM, B);
    tea_ast_node_add_child(A, tea_ast_node_create(TEA_AST_NODE_PARAM, C));
}

%syntax_error {
    fprintf(stderr, "Syntax error in parser\n");
}