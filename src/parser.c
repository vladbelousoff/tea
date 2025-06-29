#include "tea_lang.h"
#include "tea.h"  // Generated header with token constants

#include <rtl_log.h>

// External parser functions from the generated tea.c file
extern void *ParseAlloc(void *(*mallocProc)(size_t));

extern void ParseFree(void *p, void (*freeProc)(void *));

extern void Parse(void *yyp, int yymajor, ASTNode *yyminor, ASTNode **result);

// Helper function to read a file into a string
char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        rtl_log_err(, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate buffer
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        rtl_log_err(, "Error: Cannot allocate memory for file '%s'\n", filename);
        fclose(file);
        return NULL;
    }

    // Read file content
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

// Parse a TEA string
ASTNode *parse_tea_string(const char *input) {
    // Create tokenizer
    Tokenizer *tokenizer = tokenizer_create(input);
    if (!tokenizer) {
        rtl_log_err(, "Error: Failed to create tokenizer\n");
        return NULL;
    }

    // Create parser
    void *parser = ParseAlloc(malloc);
    if (!parser) {
        rtl_log_err(, "Error: Failed to create parser\n");
        tokenizer_free(tokenizer);
        return NULL;
    }

    ASTNode *result = NULL;
    bool parse_error = false;

    // Parse tokens
    while (tokenizer_has_more(tokenizer)) {
        Token token = tokenizer_next_token(tokenizer);

        if (token.type < 0) {
            // Error token
            rtl_log_err(, "Tokenizer error at line %d, column %d\n", token.line, token.column);
            parse_error = true;
            break;
        }

        if (token.type == 0) {
            // End of input
            break;
        }

        // Create token node
        ASTNode *token_node = create_node(NODE_PROGRAM, token.value);
        if (token_node) {
            token_node->line = token.line;
            token_node->column = token.column;
        }

        if (!token_node) {
            rtl_log_err(, "Failed to create token node for token %d\n", token.type);
            parse_error = true;
            break;
        }

        // Send token to parser
        Parse(parser, token.type, token_node, &result);

        // Free token value
        if (token.value) {
            free(token.value);
        }
    }

    // Send end-of-file token to parser
    if (!parse_error) {
        Parse(parser, 0, NULL, &result);
    }

    // Clean up
    ParseFree(parser, free);
    tokenizer_free(tokenizer);

    return parse_error ? NULL : result;
}

// Main parsing function
ASTNode *parse_tea_file(const char *filename) {
    // Read file content
    char *content = read_file(filename);
    if (!content) {
        return NULL;
    }

    // Parse the content
    ASTNode *result = parse_tea_string(content);

    // Clean up
    free(content);

    return result;
}
