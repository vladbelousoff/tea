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
%token GT LT EQ NE GE LE.
%token NUMBER.
%token NATIVE.
%token ARROW.
%token IF ELSE WHILE.
%token STRUCT.

program(program_node) ::= item_list(items). {
    program_node = tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (items) {
        tea_ast_node_add_children(program_node, &items->children);
        tea_ast_node_free(items);
    }
    *result = program_node;
}

item_list(item_list_node) ::= item_list(existing_items) item(new_item). {
    item_list_node = existing_items ? existing_items : tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (new_item) {
        tea_ast_node_add_child(item_list_node, new_item);
    }
}

item_list(item_list_node) ::= item(single_item). {
    item_list_node = tea_ast_node_create(TEA_AST_NODE_PROGRAM, NULL);
    if (single_item) {
        tea_ast_node_add_child(item_list_node, single_item);
    }
}

item(item_node) ::= attr_list(attrs) function(func). {
    item_node = func;
    if (attrs) {
        tea_ast_node_add_children(item_node, &attrs->children);
        tea_ast_node_free(attrs);
    }
}

item(item_node) ::= function(func). { item_node = func; }

item(item_node) ::= native_function(native_func). { item_node = native_func; }

item(item_node) ::= statement(stmt). { item_node = stmt; }

item(item_node) ::= struct_definition(struct_def). { item_node = struct_def; }

attr_list(attr_list_node) ::= attr_list(existing_attrs) attribute(new_attr). {
    attr_list_node = existing_attrs ? existing_attrs : tea_ast_node_create(TEA_AST_NODE_ATTR, NULL);
    if (new_attr) {
        tea_ast_node_add_child(attr_list_node, new_attr);
    }
}

attr_list(attr_list_node) ::= attribute(single_attr). {
    attr_list_node = tea_ast_node_create(TEA_AST_NODE_ATTR, NULL);
    if (single_attr) {
        tea_ast_node_add_child(attr_list_node, single_attr);
    }
}

attribute(attr_node) ::= AT IDENT(attr_name). {
    attr_node = tea_ast_node_create(TEA_AST_NODE_ATTR, attr_name);
}

function(func_node) ::= FN IDENT(func_name) LPAREN param_list_opt(params) RPAREN return_type_opt(return_type) LBRACE stmt_list_opt(body) RBRACE. {
    func_node = tea_ast_node_create(TEA_AST_NODE_FUNCTION, func_name);
    if (params) {
        tea_ast_node_add_child(func_node, params);
    }
    if (return_type) {
        tea_ast_node_add_child(func_node, return_type);
    }
    if (body) {
        tea_ast_node_add_child(func_node, body);
    }
}

native_function(native_func_node) ::= NATIVE FN IDENT(func_name) LPAREN param_list_opt(params) RPAREN return_type_opt(return_type) SEMICOLON. {
    native_func_node = tea_ast_node_create(TEA_AST_NODE_NATIVE_FUNCTION, func_name);
    if (params) {
        tea_ast_node_add_child(native_func_node, params);
    }
    if (return_type) {
        tea_ast_node_add_child(native_func_node, return_type);
    }
}

struct_definition(struct_def_node) ::= STRUCT IDENT(struct_name) LBRACE struct_field_list_opt(fields) RBRACE. {
    struct_def_node = tea_ast_node_create(TEA_AST_NODE_STRUCT, struct_name);
    if (fields) {
        tea_ast_node_add_child(struct_def_node, fields);
    }
}

struct_field_list_opt(field_list_node) ::= struct_field_list(fields). { field_list_node = fields; }
struct_field_list_opt(field_list_node) ::= . { field_list_node = NULL; }

struct_field_list(field_list_node) ::= struct_field_list(existing_fields) struct_field(new_field). {
    field_list_node = existing_fields ? existing_fields : tea_ast_node_create(TEA_AST_NODE_STRUCT_FIELD, NULL);
    if (new_field) {
        tea_ast_node_add_child(field_list_node, new_field);
    }
}

struct_field_list(field_list_node) ::= struct_field(single_field). {
    field_list_node = tea_ast_node_create(TEA_AST_NODE_STRUCT_FIELD, NULL);
    if (single_field) {
        tea_ast_node_add_child(field_list_node, single_field);
    }
}

struct_field(field_node) ::= IDENT(field_name) COLON IDENT(field_type) SEMICOLON. {
    field_node = tea_ast_node_create(TEA_AST_NODE_STRUCT_FIELD, field_name);
    tea_ast_node_add_child(field_node, tea_ast_node_create(TEA_AST_NODE_IDENT, field_type));
}

param_list_opt(param_list_node) ::= param_list(params). { param_list_node = params; }
param_list_opt(param_list_node) ::= . { param_list_node = NULL; }

return_type_opt(return_type_node) ::= ARROW IDENT(type_name). { return_type_node = tea_ast_node_create(TEA_AST_NODE_IDENT, type_name); }
return_type_opt(return_type_node) ::= . { return_type_node = NULL; }

type_annotation_opt(type_annotation_node) ::= COLON IDENT(type_name). { type_annotation_node = tea_ast_node_create(TEA_AST_NODE_IDENT, type_name); }
type_annotation_opt(type_annotation_node) ::= . { type_annotation_node = NULL; }

stmt_list_opt(stmt_list_node) ::= stmt_list(stmts). { stmt_list_node = stmts; }
stmt_list_opt(stmt_list_node) ::= . { stmt_list_node = NULL; }

else_opt(else_body_node) ::= ELSE LBRACE stmt_list_opt(else_stmts) RBRACE. {
    else_body_node = tea_ast_node_create(TEA_AST_NODE_ELSE, NULL);
    if (else_stmts) {
        tea_ast_node_add_child(else_body_node, else_stmts);
    }
}

else_opt(else_body_node) ::= . { else_body_node = NULL; }

param_list(param_list_node) ::= param_list(existing_params) COMMA parameter(new_param). {
    param_list_node = existing_params ? existing_params : tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (new_param) {
        tea_ast_node_add_child(param_list_node, new_param);
    }
}

param_list(param_list_node) ::= parameter(single_param). {
    param_list_node = tea_ast_node_create(TEA_AST_NODE_PARAM, NULL);
    if (single_param) {
        tea_ast_node_add_child(param_list_node, single_param);
    }
}

parameter(param_node) ::= IDENT(param_name) COLON IDENT(param_type). {
    param_node = tea_ast_node_create(TEA_AST_NODE_PARAM, param_name);
    tea_ast_node_add_child(param_node, tea_ast_node_create(TEA_AST_NODE_PARAM, param_type));
}

arg_list(arg_list_node) ::= arg_list(existing_args) COMMA expression(new_arg). {
    arg_list_node = existing_args ? existing_args : tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (new_arg) {
        tea_ast_node_add_child(arg_list_node, new_arg);
    }
}

arg_list(arg_list_node) ::= expression(single_arg). {
    arg_list_node = tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (single_arg) {
        tea_ast_node_add_child(arg_list_node, single_arg);
    }
}

stmt_list(stmt_list_node) ::= stmt_list(existing_stmts) statement(new_stmt). {
    stmt_list_node = existing_stmts ? existing_stmts : tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (new_stmt) {
        tea_ast_node_add_child(stmt_list_node, new_stmt);
    }
}

stmt_list(stmt_list_node) ::= statement(single_stmt). {
    stmt_list_node = tea_ast_node_create(TEA_AST_NODE_STMT, NULL);
    if (single_stmt) {
        tea_ast_node_add_child(stmt_list_node, single_stmt);
    }
}

statement(stmt_node) ::= let_stmt(let_stmt_node). { stmt_node = let_stmt_node; }
statement(stmt_node) ::= assign_stmt(assign_stmt_node). { stmt_node = assign_stmt_node; }
statement(stmt_node) ::= return_stmt(return_stmt_node). { stmt_node = return_stmt_node; }
statement(stmt_node) ::= if_stmt(if_stmt_node). { stmt_node = if_stmt_node; }
statement(stmt_node) ::= while_stmt(while_stmt_node). { stmt_node = while_stmt_node; }

let_stmt(let_stmt_node) ::= LET IDENT(var_name) type_annotation_opt(type_annot) ASSIGN expression(init_expr) SEMICOLON. {
    let_stmt_node = tea_ast_node_create(TEA_AST_NODE_LET, var_name);
    if (type_annot) {
        tea_ast_node_add_child(let_stmt_node, type_annot);
    }
    if (init_expr) {
        tea_ast_node_add_child(let_stmt_node, init_expr);
    }
}

let_stmt(let_stmt_node) ::= LET MUT IDENT(var_name) type_annotation_opt(type_annot) ASSIGN expression(init_expr) SEMICOLON. {
    let_stmt_node = tea_ast_node_create(TEA_AST_NODE_LET_MUT, var_name);
    if (type_annot) {
        tea_ast_node_add_child(let_stmt_node, type_annot);
    }
    if (init_expr) {
        tea_ast_node_add_child(let_stmt_node, init_expr);
    }
}

assign_stmt(assign_stmt_node) ::= IDENT(var_name) ASSIGN expression(value_expr) SEMICOLON. {
    assign_stmt_node = tea_ast_node_create(TEA_AST_NODE_ASSIGN, var_name);
    if (value_expr) {
        tea_ast_node_add_child(assign_stmt_node, value_expr);
    }
}

return_stmt(return_stmt_node) ::= RETURN SEMICOLON. {
    return_stmt_node = tea_ast_node_create(TEA_AST_NODE_RETURN, NULL);
}

return_stmt(return_stmt_node) ::= RETURN expression(return_expr) SEMICOLON. {
    return_stmt_node = tea_ast_node_create(TEA_AST_NODE_RETURN, NULL);
    if (return_expr) {
        tea_ast_node_add_child(return_stmt_node, return_expr);
    }
}

if_stmt(if_stmt_node) ::= IF expression(condition) LBRACE stmt_list_opt(then_body) RBRACE else_opt(else_body). {
    if_stmt_node = tea_ast_node_create(TEA_AST_NODE_IF, NULL);
    if (condition) {
        tea_ast_node_add_child(if_stmt_node, condition);
    }
    
    tea_ast_node_t *then_node = tea_ast_node_create(TEA_AST_NODE_THEN, NULL);
    if (then_body) {
        tea_ast_node_add_child(then_node, then_body);
    }
    tea_ast_node_add_child(if_stmt_node, then_node);
    
    if (else_body) {
        tea_ast_node_add_child(if_stmt_node, else_body);
    }
}

while_stmt(while_stmt_node) ::= WHILE expression(condition) LBRACE stmt_list_opt(loop_body) RBRACE. {
    while_stmt_node = tea_ast_node_create(TEA_AST_NODE_WHILE, NULL);
    if (condition) {
        tea_ast_node_add_child(while_stmt_node, condition);
    }
    if (loop_body) {
        tea_ast_node_add_child(while_stmt_node, loop_body);
    }
}

expression(expr_node) ::= comp_expr(comp_expr_node). { expr_node = comp_expr_node; }

comp_expr(comp_expr_node) ::= comp_expr(left_expr) GT(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= comp_expr(left_expr) LT(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= comp_expr(left_expr) EQ(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= comp_expr(left_expr) NE(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= comp_expr(left_expr) GE(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= comp_expr(left_expr) LE(op) add_expr(right_expr). {
    comp_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= add_expr(add_expr_node). { comp_expr_node = add_expr_node; }

add_expr(add_expr_node) ::= add_expr(left_expr) PLUS(op) mul_expr(right_expr). {
    add_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(add_expr_node, left_expr, right_expr);
}

add_expr(add_expr_node) ::= add_expr(left_expr) MINUS(op) mul_expr(right_expr). {
    add_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(add_expr_node, left_expr, right_expr);
}

add_expr(add_expr_node) ::= mul_expr(mul_expr_node). { add_expr_node = mul_expr_node; }

mul_expr(mul_expr_node) ::= mul_expr(left_expr) STAR(op) unary_expr(right_expr). {
    mul_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(mul_expr_node, left_expr, right_expr);
}

mul_expr(mul_expr_node) ::= mul_expr(left_expr) SLASH(op) unary_expr(right_expr). {
    mul_expr_node = tea_ast_node_create(TEA_AST_NODE_BINOP, op);
    tea_ast_node_set_binop_children(mul_expr_node, left_expr, right_expr);
}

mul_expr(mul_expr_node) ::= unary_expr(unary_expr_node). { mul_expr_node = unary_expr_node; }

unary_expr(unary_expr_node) ::= MINUS(op) unary_expr(operand). {
    unary_expr_node = tea_ast_node_create(TEA_AST_NODE_UNARY, op);
    if (operand) {
        tea_ast_node_add_child(unary_expr_node, operand);
    }
}

unary_expr(unary_expr_node) ::= primary_expr(primary_expr_node). { unary_expr_node = primary_expr_node; }

primary_expr(primary_expr_node) ::= LPAREN expression(expr) RPAREN. {
    primary_expr_node = expr;
}

primary_expr(primary_expr_node) ::= IDENT(ident_name). {
    primary_expr_node = tea_ast_node_create(TEA_AST_NODE_IDENT, ident_name);
}

primary_expr(primary_expr_node) ::= IDENT(func_name) LPAREN RPAREN. {
    primary_expr_node = tea_ast_node_create(TEA_AST_NODE_CALL, func_name);
}

primary_expr(primary_expr_node) ::= IDENT(func_name) LPAREN arg_list(args) RPAREN. {
    primary_expr_node = tea_ast_node_create(TEA_AST_NODE_CALL, func_name);
    if (args) {
        tea_ast_node_add_child(primary_expr_node, args);
    }
}

primary_expr(primary_expr_node) ::= NUMBER(number_value). {
    primary_expr_node = tea_ast_node_create(TEA_AST_NODE_NUMBER, number_value);
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