#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct Token Token;
typedef struct Tokenizer Tokenizer;

// AST node types - simplified to only what we need
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_PARAM,
    NODE_ATTR
} NodeType;

// AST node structure
struct ASTNode {
    NodeType type;
    char *value;
    ASTNode **children;
    int child_count;
    int line;
    int column;
};

// Token structure
struct Token {
    int type;           // Token type
    char *value;        // Token value
    int line;           // Line number
    int column;         // Column number
};

// Tokenizer structure
struct Tokenizer {
    const char *input;  // Input string
    int pos;            // Current position
    int line;           // Current line
    int column;         // Current column
    int length;         // Input length
    Token current_token;
};

// AST Functions
ASTNode* create_node(NodeType type, const char *value);
void add_child(ASTNode *parent, ASTNode *child);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

// Tokenizer Functions
Tokenizer* tokenizer_create(const char *input);
void tokenizer_free(Tokenizer *tokenizer);
Token tokenizer_next_token(Tokenizer *tokenizer);
bool tokenizer_has_more(Tokenizer *tokenizer);

// Parser Integration
void* ParseAlloc(void* (*allocProc)(size_t));
void ParseFree(void* parser, void (*freeProc)(void*));
void Parse(void* parser, int tokenType, ASTNode* token, ASTNode** result);

// File Processing
char* read_file(const char *filename);
ASTNode* parse_tea_file(const char *filename);
ASTNode* parse_tea_string(const char *input);

