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
%token LET MUT SEMICOLON ASSIGN.
%token MINUS PLUS STAR.
%token NUMBER.

program(A) ::= item_list(B). {
    A = tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (B) {
        tea_ast_node_add_children(A, &B->children);
        tea_ast_node_free(B);
    }
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
    if (B) {
        tea_ast_node_add_children(A, &B->children);
        tea_ast_node_free(B);
    }
}

item(A) ::= function(B). { A = B; }

item(A) ::= statement(B). { A = B; }

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

function(A) ::= FN IDENT(B) LPAREN RPAREN LBRACE RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
}

function(A) ::= FN IDENT(B) LPAREN param_list(C) RPAREN LBRACE RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
}

function(A) ::= FN IDENT(B) LPAREN RPAREN LBRACE stmt_list(C) RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
}

function(A) ::= FN IDENT(B) LPAREN param_list(C) RPAREN LBRACE stmt_list(D) RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
    if (D) tea_ast_node_add_child(A, D);
}

param_list(A) ::= param_list(B) COMMA parameter(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

param_list(A) ::= parameter(B). {
    A = tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

parameter(A) ::= IDENT(B) COLON IDENT(C). {
    A = tea_ast_node_create(TEA_AST_NODE_PARAM, B);
    tea_ast_node_add_child(A, tea_ast_node_create(TEA_AST_NODE_PARAM, C));
}

stmt_list(A) ::= stmt_list(B) statement(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

stmt_list(A) ::= statement(B). {
    A = tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

statement(A) ::= let_stmt(B). { A = B; }
statement(A) ::= assign_stmt(B). { A = B; }

let_stmt(A) ::= LET IDENT(B) ASSIGN expression(C) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_LET, B);
    if (C) tea_ast_node_add_child(A, C);
}

let_stmt(A) ::= LET MUT IDENT(B) ASSIGN expression(C) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_LET_MUT, B);
    if (C) tea_ast_node_add_child(A, C);
}

assign_stmt(A) ::= IDENT(B) ASSIGN expression(C) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_ASSIGN, B);
    if (C) tea_ast_node_add_child(A, C);
}

expression(A) ::= add_expr(B). { A = B; }

add_expr(A) ::= add_expr(B) PLUS(D) mul_expr(C). {
    A = tea_ast_node_create(TEA_AST_NODE_BINOP, D);
    tea_ast_node_set_binop_children(A, B, C);
}

add_expr(A) ::= add_expr(B) MINUS(D) mul_expr(C). {
    A = tea_ast_node_create(TEA_AST_NODE_BINOP, D);
    tea_ast_node_set_binop_children(A, B, C);
}

add_expr(A) ::= mul_expr(B). { A = B; }

mul_expr(A) ::= mul_expr(B) STAR(D) unary_expr(C). {
    A = tea_ast_node_create(TEA_AST_NODE_BINOP, D);
    tea_ast_node_set_binop_children(A, B, C);
}

mul_expr(A) ::= unary_expr(B). { A = B; }

unary_expr(A) ::= MINUS(D) unary_expr(B). {
    A = tea_ast_node_create(TEA_AST_NODE_UNARY, D);
    if (B) tea_ast_node_add_child(A, B);
}

unary_expr(A) ::= primary_expr(B). { A = B; }

primary_expr(A) ::= LPAREN expression(B) RPAREN. {
    A = B;
}

primary_expr(A) ::= IDENT(B). {
    A = tea_ast_node_create(TEA_AST_NODE_IDENT, B);
}

primary_expr(A) ::= NUMBER(B). {
    A = tea_ast_node_create(TEA_AST_NODE_NUMBER, B);
}

%syntax_error {
    if (yyminor) {
        fprintf(stderr, "Syntax error: Unexpected token '%.*s' at line %d, column %d\n",
                yyminor->buffer_size, yyminor->buffer, yyminor->line, yyminor->column);
        fprintf(stderr, "Token type: %s\n", tea_get_token_name(yymajor));
    } else {
        fprintf(stderr, "Syntax error: Unexpected end of input\n");
    }
}