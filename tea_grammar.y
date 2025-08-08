%include
{
#include <stdlib.h>
#include <assert.h>
#include "tea_ast.h"
#include <rtl_log.h>
}

%token_type {tea_tok_t*}
%default_type {tea_node_t*}
%extra_argument {tea_node_t **result}

%token_prefix TEA_TOKEN_

%token FN IDENT LPAREN RPAREN LBRACE RBRACE.
%token AT COLON COMMA.
%token LET MUT SEMICOLON ASSIGN.
%token RETURN.
%token MINUS PLUS STAR SLASH.
%token GT LT EQ NE GE LE.
%token AND OR.
%token INTEGER_NUMBER FLOAT_NUMBER.
%token STRING.
%token ARROW.
%token IF ELSE WHILE BREAK CONTINUE.
%token STRUCT.
%token IMPL.
%token NEW.
%token DOT.
%token EXCLAMATION_MARK QUESTION_MARK.
%token NULL.
%token TRAIT.
%token FOR.

%left ASSIGN.
%left PLUS MINUS.
%left STAR SLASH.
%left DOT.
%left LPAREN.

program(program_node) ::= item_list(items). {
    program_node = tea_node_create(TEA_N_PROG, NULL);
    if (items) {
        tea_node_add_children(program_node, &items->children);
        tea_node_free(items);
    }
    *result = program_node;
}

item_list(item_list_node) ::= item_list(existing_items) item(new_item). {
    item_list_node = existing_items ? existing_items : tea_node_create(TEA_N_PROG, NULL);
    tea_node_add_child(item_list_node, new_item);
}

item_list(item_list_node) ::= item(single_item). {
    item_list_node = tea_node_create(TEA_N_PROG, NULL);
    tea_node_add_child(item_list_node, single_item);
}

item(item_node) ::= attr_list(attrs) function(func). {
    item_node = func;
    if (attrs) {
        tea_node_add_children(item_node, &attrs->children);
        tea_node_free(attrs);
    }
}

item(item_node) ::= function(func). { item_node = func; }
item(item_node) ::= statement(stmt). { item_node = stmt; }
item(item_node) ::= struct_definition(struct_def). { item_node = struct_def; }
item(item_node) ::= impl_block(impl_block_node). { item_node = impl_block_node; }
item(item_node) ::= trait_definition(trait_def). { item_node = trait_def; }
item(item_node) ::= trait_impl_block(trait_impl_node). { item_node = trait_impl_node; }

attr_list(attr_list_node) ::= attr_list(existing_attrs) attribute(new_attr). {
    attr_list_node = existing_attrs ? existing_attrs : tea_node_create(TEA_N_ATTR, NULL);
    tea_node_add_child(attr_list_node, new_attr);
}

attr_list(attr_list_node) ::= attribute(single_attr). {
    attr_list_node = tea_node_create(TEA_N_ATTR, NULL);
    tea_node_add_child(attr_list_node, single_attr);
}

attribute(attr_node) ::= AT IDENT(attr_name). {
    attr_node = tea_node_create(TEA_N_ATTR, attr_name);
}

function(func_node) ::= function_header(header) function_body(body). {
    func_node = header;
    tea_node_add_child(func_node, body);
}

function_header(header_node) ::= FN mut_opt(mut) IDENT(func_name) LPAREN param_list_opt(params) RPAREN return_type_opt(return_type). {
    header_node = tea_node_create(TEA_N_FN, func_name);
    tea_node_add_child(header_node, mut);
    tea_node_add_child(header_node, params);
    tea_node_add_child(header_node, return_type);
}

mut_opt(mut_node) ::= MUT. { mut_node = tea_node_create(TEA_N_MUT, NULL); }
mut_opt(mut_node) ::= . { mut_node = NULL; }

function_body(body_node) ::= LBRACE stmt_list_opt(stmts) RBRACE. { body_node = stmts; }
function_body(body_node) ::= SEMICOLON. { body_node = NULL; }

struct_definition(struct_def_node) ::= STRUCT IDENT(struct_name) LBRACE struct_field_list_opt(fields) RBRACE. {
    struct_def_node = tea_node_create(TEA_N_STRUCT, struct_name);
    if (fields) {
        tea_node_add_children(struct_def_node, &fields->children);
        tea_node_free(fields);
    }
}

struct_field_list_opt(field_list_node) ::= struct_field_list(fields). { field_list_node = fields; }
struct_field_list_opt(field_list_node) ::= . { field_list_node = NULL; }

struct_field_list(field_list_node) ::= struct_field_list(existing_fields) struct_field(new_field). {
    field_list_node = existing_fields ? existing_fields : tea_node_create(TEA_N_STRUCT_FIELD, NULL);
    tea_node_add_child(field_list_node, new_field);
}

struct_field_list(field_list_node) ::= struct_field(single_field). {
    field_list_node = tea_node_create(TEA_N_STRUCT_FIELD, NULL);
    tea_node_add_child(field_list_node, single_field);
}

struct_field(field_node) ::= IDENT(field_name) COLON type_spec(field_type_node) SEMICOLON. {
    field_node = tea_node_create(TEA_N_STRUCT_FIELD, field_name);
    tea_node_add_child(field_node, field_type_node);
}

impl_block(impl_block_node) ::= IMPL IDENT(struct_name) LBRACE impl_method_list_opt(methods) RBRACE. {
    impl_block_node = tea_node_create(TEA_N_IMPL_BLK, struct_name);
    if (methods) {
        tea_node_add_children(impl_block_node, &methods->children);
        tea_node_free(methods);
    }
}

impl_method_list_opt(method_list_node) ::= impl_method_list(methods). { method_list_node = methods; }
impl_method_list_opt(method_list_node) ::= . { method_list_node = NULL; }

impl_method_list(method_list_node) ::= impl_method_list(existing_methods) impl_method(new_method). {
    method_list_node = existing_methods ? existing_methods : tea_node_create(TEA_N_IMPL_ITEM, NULL);
    tea_node_add_child(method_list_node, new_method);
}

impl_method_list(method_list_node) ::= impl_method(single_method). {
    method_list_node = tea_node_create(TEA_N_IMPL_ITEM, NULL);
    tea_node_add_child(method_list_node, single_method);
}

impl_method(method_node) ::= function_header(header) function_body(body). {
    method_node = tea_node_create(TEA_N_IMPL_ITEM, NULL);
    tea_node_add_child(method_node, header);
    tea_node_add_child(header, body);
}

trait_definition(trait_def_node) ::= TRAIT IDENT(trait_name) LBRACE trait_method_list_opt(methods) RBRACE. {
    trait_def_node = tea_node_create(TEA_N_TRAIT, trait_name);
    if (methods) {
        tea_node_add_children(trait_def_node, &methods->children);
        tea_node_free(methods);
    }
}

trait_method_list_opt(method_list_node) ::= trait_method_list(methods). { method_list_node = methods; }
trait_method_list_opt(method_list_node) ::= . { method_list_node = NULL; }

trait_method_list(method_list_node) ::= trait_method_list(existing_methods) trait_method(new_method). {
    method_list_node = existing_methods ? existing_methods : tea_node_create(TEA_N_TRAIT_METHOD, NULL);
    tea_node_add_child(method_list_node, new_method);
}

trait_method_list(method_list_node) ::= trait_method(single_method). {
    method_list_node = tea_node_create(TEA_N_TRAIT_METHOD, NULL);
    tea_node_add_child(method_list_node, single_method);
}

trait_method(method_node) ::= function_header(header) SEMICOLON. {
    method_node = tea_node_create(TEA_N_TRAIT_METHOD, NULL);
    tea_node_add_child(method_node, header);
}

trait_impl_block(trait_impl_node) ::= IMPL IDENT(trait_name) FOR IDENT(struct_name) LBRACE trait_impl_method_list_opt(methods) RBRACE. {
    trait_impl_node = tea_node_create(TEA_N_TRAIT_IMPL, trait_name);
    tea_node_t* struct_name_node = tea_node_create(TEA_N_IDENT, struct_name);
    tea_node_add_child(trait_impl_node, struct_name_node);
    if (methods) {
        tea_node_add_children(trait_impl_node, &methods->children);
        tea_node_free(methods);
    }
}

trait_impl_method_list_opt(method_list_node) ::= trait_impl_method_list(methods). { method_list_node = methods; }
trait_impl_method_list_opt(method_list_node) ::= . { method_list_node = NULL; }

trait_impl_method_list(method_list_node) ::= trait_impl_method_list(existing_methods) trait_impl_method(new_method). {
    method_list_node = existing_methods ? existing_methods : tea_node_create(TEA_N_TRAIT_IMPL_ITEM, NULL);
    tea_node_add_child(method_list_node, new_method);
}

trait_impl_method_list(method_list_node) ::= trait_impl_method(single_method). {
    method_list_node = tea_node_create(TEA_N_TRAIT_IMPL_ITEM, NULL);
    tea_node_add_child(method_list_node, single_method);
}

trait_impl_method(method_node) ::= function_header(header) function_body(body). {
    method_node = tea_node_create(TEA_N_TRAIT_IMPL_ITEM, NULL);
    tea_node_add_child(method_node, header);
    tea_node_add_child(header, body);
}

struct_field_init_list_opt(field_init_list_node) ::= struct_field_init_list(field_inits). { field_init_list_node = field_inits; }
struct_field_init_list_opt(field_init_list_node) ::= . { field_init_list_node = NULL; }

struct_field_init_list(field_init_list_node) ::= struct_field_init_list(existing_inits) struct_field_init(new_init). {
    field_init_list_node = existing_inits ? existing_inits : tea_node_create(TEA_N_STRUCT_INIT, NULL);
    tea_node_add_child(field_init_list_node, new_init);
}

struct_field_init_list(field_init_list_node) ::= struct_field_init(single_init). {
    field_init_list_node = tea_node_create(TEA_N_STRUCT_INIT, NULL);
    tea_node_add_child(field_init_list_node, single_init);
}

struct_field_init(field_init_node) ::= IDENT(field_name) COLON expression(value_expr). {
    field_init_node = tea_node_create(TEA_N_STRUCT_INIT, field_name);
    tea_node_add_child(field_init_node, value_expr);
}

struct_field_init(field_init_node) ::= IDENT(field_name) COLON expression(value_expr) COMMA. {
    field_init_node = tea_node_create(TEA_N_STRUCT_INIT, field_name);
    tea_node_add_child(field_init_node, value_expr);
}

param_list_opt(param_list_node) ::= param_list(params). { param_list_node = params; }
param_list_opt(param_list_node) ::= . { param_list_node = NULL; }

return_type_opt(return_type_node) ::= ARROW type_spec(type_node). { return_type_node = tea_node_create(TEA_N_RET_TYPE, NULL); tea_node_add_child(return_type_node, type_node); }
return_type_opt(return_type_node) ::= . { return_type_node = NULL; }

type_annotation_opt(type_annotation_node) ::= COLON type_spec(type_node). { type_annotation_node = tea_node_create(TEA_N_TYPE_ANNOT, NULL); tea_node_add_child(type_annotation_node, type_node); }
type_annotation_opt(type_annotation_node) ::= . { type_annotation_node = NULL; }

type_spec(type_spec_node) ::= IDENT(type_name) QUESTION_MARK. {
    type_spec_node = tea_node_create(TEA_N_OPT_TYPE, type_name);
}

type_spec(type_spec_node) ::= IDENT(type_name) . {
    type_spec_node = tea_node_create(TEA_N_IDENT, type_name);
}

stmt_list_opt(stmt_list_node) ::= stmt_list(stmts). { stmt_list_node = stmts; }
stmt_list_opt(stmt_list_node) ::= . { stmt_list_node = NULL; }

else_opt(else_body_node) ::= ELSE LBRACE stmt_list_opt(else_stmts) RBRACE. {
    else_body_node = tea_node_create(TEA_N_ELSE, NULL);
    tea_node_add_child(else_body_node, else_stmts);
}

else_opt(else_body_node) ::= . { else_body_node = NULL; }

param_list(param_list_node) ::= param_list(existing_params) COMMA parameter(new_param). {
    param_list_node = existing_params ? existing_params : tea_node_create(TEA_N_PARAM, NULL);
    tea_node_add_child(param_list_node, new_param);
}

param_list(param_list_node) ::= parameter(single_param). {
    param_list_node = tea_node_create(TEA_N_PARAM, NULL);
    tea_node_add_child(param_list_node, single_param);
}

parameter(param_node) ::= IDENT(param_name) COLON type_spec(type_node). {
    param_node = tea_node_create(TEA_N_PARAM, param_name);
    tea_node_add_child(param_node, type_node);
}

arg_list(arg_list_node) ::= arg_list(existing_args) COMMA expression(new_arg). {
    arg_list_node = existing_args ? existing_args : tea_node_create(TEA_N_FN_ARGS, NULL);
    tea_node_add_child(arg_list_node, new_arg);
}

arg_list(arg_list_node) ::= expression(single_arg). {
    arg_list_node = tea_node_create(TEA_N_FN_ARGS, NULL);
    tea_node_add_child(arg_list_node, single_arg);
}

arg_list_opt(arg_list_node) ::= arg_list(args). { arg_list_node = args; }
arg_list_opt(arg_list_node) ::= . { arg_list_node = NULL; }

stmt_list(stmt_list_node) ::= stmt_list(existing_stmts) statement(new_stmt). {
    stmt_list_node = existing_stmts ? existing_stmts : tea_node_create(TEA_N_STMT, NULL);
    tea_node_add_child(stmt_list_node, new_stmt);
}

stmt_list(stmt_list_node) ::= statement(single_stmt). {
    stmt_list_node = tea_node_create(TEA_N_STMT, NULL);
    tea_node_add_child(stmt_list_node, single_stmt);
}

statement(stmt_node) ::= let_stmt(let_stmt_node). { stmt_node = let_stmt_node; }
statement(stmt_node) ::= assign_stmt(assign_stmt_node). { stmt_node = assign_stmt_node; }
statement(stmt_node) ::= return_stmt(return_stmt_node). { stmt_node = return_stmt_node; }
statement(stmt_node) ::= break_stmt(break_stmt_node). { stmt_node = break_stmt_node; }
statement(stmt_node) ::= continue_stmt(continue_stmt_node). { stmt_node = continue_stmt_node; }
statement(stmt_node) ::= if_stmt(if_stmt_node). { stmt_node = if_stmt_node; }
statement(stmt_node) ::= while_stmt(while_stmt_node). { stmt_node = while_stmt_node; }
statement(stmt_node) ::= expression(expr_node) SEMICOLON. { stmt_node = expr_node; }

let_stmt(let_stmt_node) ::= LET mut_opt(mut) IDENT(var_name) type_annotation_opt(type_annot) ASSIGN expression(init_expr) SEMICOLON. {
    let_stmt_node = tea_node_create(TEA_N_LET, var_name);
    tea_node_add_child(let_stmt_node, mut);
    tea_node_add_child(let_stmt_node, type_annot);
    tea_node_add_child(let_stmt_node, init_expr);
}

assign_stmt(assign_stmt_node) ::= IDENT(ident) ASSIGN expression(rhs) SEMICOLON. {
    assign_stmt_node = tea_node_create(TEA_N_ASSIGN, NULL);
    tea_node_t* lhs = tea_node_create(TEA_N_IDENT, ident);
    tea_node_set_binop(assign_stmt_node, lhs, rhs);
}

assign_stmt(assign_stmt_node) ::= field_access(lhs) ASSIGN expression(rhs) SEMICOLON. {
    assign_stmt_node = tea_node_create(TEA_N_ASSIGN, NULL);
    tea_node_set_binop(assign_stmt_node, lhs, rhs);
}

return_stmt(return_stmt_node) ::= RETURN SEMICOLON. {
    return_stmt_node = tea_node_create(TEA_N_RET, NULL);
}

return_stmt(return_stmt_node) ::= RETURN expression(return_expr) SEMICOLON. {
    return_stmt_node = tea_node_create(TEA_N_RET, NULL);
    tea_node_add_child(return_stmt_node, return_expr);
}

break_stmt(break_stmt_node) ::= BREAK SEMICOLON. {
    break_stmt_node = tea_node_create(TEA_N_BREAK, NULL);
}

continue_stmt(continue_stmt_node) ::= CONTINUE SEMICOLON. {
    continue_stmt_node = tea_node_create(TEA_N_CONT, NULL);
}

if_stmt(if_stmt_node) ::= IF expression(condition) LBRACE stmt_list_opt(then_body) RBRACE else_opt(else_body). {
    if_stmt_node = tea_node_create(TEA_N_IF, NULL);
    tea_node_add_child(if_stmt_node, condition);
    
    tea_node_t *then_node = tea_node_create(TEA_N_THEN, NULL);
    tea_node_add_child(then_node, then_body);
    tea_node_add_child(if_stmt_node, then_node);
    
    tea_node_add_child(if_stmt_node, else_body);
}

while_stmt(while_stmt_node) ::= WHILE expression(condition) LBRACE stmt_list_opt(loop_body) RBRACE. {
    while_stmt_node = tea_node_create(TEA_N_WHILE, NULL);

    tea_node_t *cond_node = tea_node_create(TEA_N_WHILE_COND, NULL);
    tea_node_add_child(cond_node, condition);

    tea_node_add_child(while_stmt_node, cond_node);
    
    tea_node_t *body_node = tea_node_create(TEA_N_WHILE_BODY, NULL);
    tea_node_add_child(body_node, loop_body);

    tea_node_add_child(while_stmt_node, body_node);
}

expression(expr_node) ::= logical_expr(logical_expr_node). { expr_node = logical_expr_node; }

logical_expr(logical_expr_node) ::= logical_expr(left_expr) AND|OR(op) comp_expr(right_expr). {
    logical_expr_node = tea_node_create(TEA_N_BINOP, op);
    tea_node_set_binop(logical_expr_node, left_expr, right_expr);
}

logical_expr(logical_expr_node) ::= comp_expr(comp_expr_node). { logical_expr_node = comp_expr_node; }

comp_expr(comp_expr_node) ::= comp_expr(left_expr) GT|LT|EQ|NE|GE|LE(op) add_expr(right_expr). {
    comp_expr_node = tea_node_create(TEA_N_BINOP, op);
    tea_node_set_binop(comp_expr_node, left_expr, right_expr);
}

comp_expr(comp_expr_node) ::= add_expr(add_expr_node). { comp_expr_node = add_expr_node; }

add_expr(add_expr_node) ::= add_expr(left_expr) PLUS|MINUS(op) mul_expr(right_expr). {
    add_expr_node = tea_node_create(TEA_N_BINOP, op);
    tea_node_set_binop(add_expr_node, left_expr, right_expr);
}

add_expr(add_expr_node) ::= mul_expr(mul_expr_node). { add_expr_node = mul_expr_node; }

mul_expr(mul_expr_node) ::= mul_expr(left_expr) STAR|SLASH(op) unary_expr(right_expr). {
    mul_expr_node = tea_node_create(TEA_N_BINOP, op);
    tea_node_set_binop(mul_expr_node, left_expr, right_expr);
}

mul_expr(mul_expr_node) ::= unary_expr(unary_expr_node). { mul_expr_node = unary_expr_node; }

unary_expr(unary_expr_node) ::= PLUS|MINUS|EXCLAMATION_MARK(op) unary_expr(operand). {
    unary_expr_node = tea_node_create(TEA_N_UNARY, op);
    tea_node_add_child(unary_expr_node, operand);
}

unary_expr(unary_expr_node) ::= primary_expr(primary_expr_node). { unary_expr_node = primary_expr_node; }

primary_expr(primary_expr_node) ::= LPAREN expression(expr) RPAREN. {
    primary_expr_node = expr;
}

primary_expr(primary_expr_node) ::= IDENT(ident_name). {
    primary_expr_node = tea_node_create(TEA_N_IDENT, ident_name);
}

primary_expr(primary_expr_node) ::= IDENT(func_name) LPAREN arg_list_opt(args) RPAREN. {
    primary_expr_node = tea_node_create(TEA_N_FN_CALL, func_name);
    tea_node_add_child(primary_expr_node, args);
}

primary_expr(primary_expr_node) ::= INTEGER_NUMBER(number_value). {
    primary_expr_node = tea_node_create(TEA_N_INT, number_value);
}

primary_expr(primary_expr_node) ::= FLOAT_NUMBER(number_value). {
    primary_expr_node = tea_node_create(TEA_N_FLOAT, number_value);
}

primary_expr(primary_expr_node) ::= NEW IDENT(struct_type) LBRACE struct_field_init_list_opt(field_inits) RBRACE. {
    primary_expr_node = tea_node_create(TEA_N_STRUCT_INST, struct_type);
    if (field_inits) {
        tea_node_add_children(primary_expr_node, &field_inits->children);
        tea_node_free(field_inits);
    }
}

primary_expr(primary_expr_node) ::= STRING(string_value). {
    primary_expr_node = tea_node_create(TEA_N_STR, string_value);
}

primary_expr(primary_expr_node) ::= NULL. {
    primary_expr_node = tea_node_create(TEA_N_NULL, NULL);
}

primary_expr(primary_expr_node) ::= primary_expr(object_expr) DOT IDENT(method_name) LPAREN arg_list_opt(args) RPAREN. {
    // Create a method call represented as a function call with field access
    primary_expr_node = tea_node_create(TEA_N_FN_CALL, NULL);
    
    // Create field access for object.method
    tea_node_t* method_access = tea_node_create(TEA_N_FIELD_ACC, NULL);
    tea_node_t* method_node = tea_node_create(TEA_N_IDENT, method_name);
    tea_node_set_field_acc(method_access, object_expr, method_node);
    
    // Add the method access as the first child
    tea_node_add_child(primary_expr_node, method_access);
    
    // Add arguments if present
    tea_node_add_child(primary_expr_node, args);
}

primary_expr(primary_expr_node) ::= field_access(field_access_node). {
    primary_expr_node = field_access_node;
}

field_access(field_access_node) ::= primary_expr(object_expr) DOT IDENT(field_name). {
    field_access_node = tea_node_create(TEA_N_FIELD_ACC, NULL);
    tea_node_t* field_node = tea_node_create(TEA_N_IDENT, field_name);
    tea_node_set_field_acc(field_access_node, object_expr, field_node);
}

%syntax_error {
    if (yyminor) {
        rtl_log_err("Syntax error: Unexpected token <%s> '%.*s' at line %d, column %d",
                tea_tok_name(yymajor),
                yyminor->size,
                yyminor->buf,
                yyminor->line,
                yyminor->col);
    } else {
        rtl_log_err("Syntax error: Unexpected end of input");
    }
    exit(1);
}