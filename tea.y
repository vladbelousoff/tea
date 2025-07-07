%include
{
#include <stdlib.h>
#include <assert.h>
#include "tea_ast.h"
#include <rtl_log.h>
}

%token_type {tea_token_t*}
%default_type {tea_ast_node_t*}
%extra_argument {tea_ast_node_t **result}

%token_prefix TEA_TOKEN_

%token FN IDENT LPAREN RPAREN LBRACE RBRACE.
%token AT COLON COMMA.
%token LET MUT SEMICOLON ASSIGN.
%token RETURN.
%token MINUS PLUS STAR SLASH.
%token GT LT.
%token NUMBER.
%token NATIVE.
%token ARROW.
%token IF ELSE WHILE.

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

item(A) ::= native_function(B). { A = B; }

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

function(A) ::= FN IDENT(B) LPAREN param_list_opt(C) RPAREN return_type_opt(D) LBRACE stmt_list_opt(E) RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
    if (D) tea_ast_node_add_child(A, D);
    if (E) tea_ast_node_add_child(A, E);
}

native_function(A) ::= NATIVE FN IDENT(B) LPAREN param_list_opt(C) RPAREN return_type_opt(D) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_NATIVE_FUNCTION, B);
    if (C) tea_ast_node_add_child(A, C);
    if (D) tea_ast_node_add_child(A, D);
}

param_list_opt(A) ::= param_list(B). { A = B; }
param_list_opt(A) ::= . { A = NULL; }

return_type_opt(A) ::= ARROW IDENT(B). { A = tea_ast_node_create(TEA_AST_NODE_IDENT, B); }
return_type_opt(A) ::= . { A = NULL; }

type_annotation_opt(A) ::= COLON IDENT(B). { A = tea_ast_node_create(TEA_AST_NODE_IDENT, B); }
type_annotation_opt(A) ::= . { A = NULL; }

stmt_list_opt(A) ::= stmt_list(B). { A = B; }
stmt_list_opt(A) ::= . { A = NULL; }

else_opt(A) ::= ELSE LBRACE stmt_list_opt(B) RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_ELSE, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

else_opt(A) ::= . { A = NULL; }

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

arg_list(A) ::= arg_list(B) COMMA expression(C). {
    A = B ? B : tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (C) tea_ast_node_add_child(A, C);
}

arg_list(A) ::= expression(B). {
    A = tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (B) tea_ast_node_add_child(A, B);
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
statement(A) ::= return_stmt(B). { A = B; }
statement(A) ::= if_stmt(B). { A = B; }
statement(A) ::= while_stmt(B). { A = B; }

let_stmt(A) ::= LET IDENT(B) type_annotation_opt(C) ASSIGN expression(D) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_LET, B);
    if (C) tea_ast_node_add_child(A, C);
    if (D) tea_ast_node_add_child(A, D);
}

let_stmt(A) ::= LET MUT IDENT(B) type_annotation_opt(C) ASSIGN expression(D) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_LET_MUT, B);
    if (C) tea_ast_node_add_child(A, C);
    if (D) tea_ast_node_add_child(A, D);
}

assign_stmt(A) ::= IDENT(B) ASSIGN expression(C) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_ASSIGN, B);
    if (C) tea_ast_node_add_child(A, C);
}

return_stmt(A) ::= RETURN SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_RETURN, NULL);
}

return_stmt(A) ::= RETURN expression(B) SEMICOLON. {
    A = tea_ast_node_create(TEA_AST_NODE_RETURN, NULL);
    if (B) tea_ast_node_add_child(A, B);
}

if_stmt(A) ::= IF expression(B) LBRACE stmt_list_opt(C) RBRACE else_opt(D). {
    A = tea_ast_node_create(TEA_AST_NODE_IF, NULL);
    if (B) tea_ast_node_add_child(A, B);
    
    tea_ast_node_t *then_node = tea_ast_node_create(TEA_AST_NODE_THEN, NULL);
    if (C) tea_ast_node_add_child(then_node, C);
    tea_ast_node_add_child(A, then_node);
    
    if (D) tea_ast_node_add_child(A, D);
}

while_stmt(A) ::= WHILE expression(B) LBRACE stmt_list_opt(C) RBRACE. {
    A = tea_ast_node_create(TEA_AST_NODE_WHILE, NULL);
    if (B) tea_ast_node_add_child(A, B);
    if (C) tea_ast_node_add_child(A, C);
}

expression(A) ::= comp_expr(B). { A = B; }

comp_expr(A) ::= comp_expr(B) GT(D) add_expr(C). {
    A = tea_ast_node_create(TEA_AST_NODE_BINOP, D);
    tea_ast_node_set_binop_children(A, B, C);
}

comp_expr(A) ::= comp_expr(B) LT(D) add_expr(C). {
    A = tea_ast_node_create(TEA_AST_NODE_BINOP, D);
    tea_ast_node_set_binop_children(A, B, C);
}

comp_expr(A) ::= add_expr(B). { A = B; }

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

mul_expr(A) ::= mul_expr(B) SLASH(D) unary_expr(C). {
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

primary_expr(A) ::= IDENT(B) LPAREN RPAREN. {
    A = tea_ast_node_create(TEA_AST_NODE_CALL, B);
}

primary_expr(A) ::= IDENT(B) LPAREN arg_list(C) RPAREN. {
    A = tea_ast_node_create(TEA_AST_NODE_CALL, B);
    if (C) tea_ast_node_add_child(A, C);
}

primary_expr(A) ::= NUMBER(B). {
    A = tea_ast_node_create(TEA_AST_NODE_NUMBER, B);
}

%syntax_error {
    if (yyminor) {
        rtl_log_err("Syntax error: Unexpected token <%s> '%.*s' at line %d, column %d",
                tea_get_token_name(yymajor),
                yyminor->buffer_size,
                yyminor->buffer,
                yyminor->line,
                yyminor->column);
    } else {
        rtl_log_err("Syntax error: Unexpected end of input");
    }
    exit(1);
}