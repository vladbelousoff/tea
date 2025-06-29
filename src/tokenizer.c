#include <ctype.h>
#include <string.h>

#include "tea_lang.h"
#include "tea.h"  // Generated header with token constants

// Keywords table - only 'fn' needed
typedef struct {
    const char *keyword;
    int token_type;
} KeywordEntry;

static const KeywordEntry keywords[] = {
    {"fn", TEA_TOKEN_FN},
    {NULL, 0} // Sentinel
};

// Helper function to skip whitespace
static void skip_whitespace(Tokenizer *tokenizer) {
    while (tokenizer->pos < tokenizer->length) {
        char c = tokenizer->input[tokenizer->pos];
        if (c == ' ' || c == '\t') {
            tokenizer->pos++;
            tokenizer->column++;
        } else if (c == '\n') {
            tokenizer->pos++;
            tokenizer->line++;
            tokenizer->column = 1;
        } else if (c == '\r') {
            tokenizer->pos++;
            if (tokenizer->pos < tokenizer->length && tokenizer->input[tokenizer->pos] == '\n') {
                tokenizer->pos++;
            }
            tokenizer->line++;
            tokenizer->column = 1;
        } else {
            break;
        }
    }
}

// Helper function to skip line comments
static void skip_line_comment(Tokenizer *tokenizer) {
    // Skip // comments
    if (tokenizer->pos + 1 < tokenizer->length &&
        tokenizer->input[tokenizer->pos] == '/' &&
        tokenizer->input[tokenizer->pos + 1] == '/') {
        tokenizer->pos += 2;
        tokenizer->column += 2;

        // Skip until end of line
        while (tokenizer->pos < tokenizer->length &&
               tokenizer->input[tokenizer->pos] != '\n' &&
               tokenizer->input[tokenizer->pos] != '\r') {
            tokenizer->pos++;
            tokenizer->column++;
        }
    }
}

// Helper function to skip block comments
static void skip_block_comment(Tokenizer *tokenizer) {
    // Skip /* ... */ comments
    if (tokenizer->pos + 1 < tokenizer->length &&
        tokenizer->input[tokenizer->pos] == '/' &&
        tokenizer->input[tokenizer->pos + 1] == '*') {
        tokenizer->pos += 2;
        tokenizer->column += 2;

        // Skip until */
        while (tokenizer->pos + 1 < tokenizer->length) {
            if (tokenizer->input[tokenizer->pos] == '*' &&
                tokenizer->input[tokenizer->pos + 1] == '/') {
                tokenizer->pos += 2;
                tokenizer->column += 2;
                break;
            } else if (tokenizer->input[tokenizer->pos] == '\n') {
                tokenizer->pos++;
                tokenizer->line++;
                tokenizer->column = 1;
            } else {
                tokenizer->pos++;
                tokenizer->column++;
            }
        }
    }
}

// Helper function to read an identifier
static char *read_identifier(Tokenizer *tokenizer) {
    int start = tokenizer->pos;

    // First character must be letter or underscore
    if (!isalpha(tokenizer->input[tokenizer->pos]) && tokenizer->input[tokenizer->pos] != '_') {
        return NULL;
    }

    // Read alphanumeric characters and underscores
    while (tokenizer->pos < tokenizer->length &&
           (isalnum(tokenizer->input[tokenizer->pos]) || tokenizer->input[tokenizer->pos] == '_')) {
        tokenizer->pos++;
        tokenizer->column++;
    }

    int length = tokenizer->pos - start;
    char *identifier = malloc(length + 1);
    strncpy(identifier, &tokenizer->input[start], length);
    identifier[length] = '\0';

    return identifier;
}

// Helper function to read a number literal
static char *read_number(Tokenizer *tokenizer) {
    int start = tokenizer->pos;

    // First character must be digit
    if (!isdigit(tokenizer->input[tokenizer->pos])) {
        return NULL;
    }

    // Read digits
    while (tokenizer->pos < tokenizer->length && isdigit(tokenizer->input[tokenizer->pos])) {
        tokenizer->pos++;
        tokenizer->column++;
    }

    // Check for decimal point (float)
    if (tokenizer->pos < tokenizer->length && tokenizer->input[tokenizer->pos] == '.') {
        tokenizer->pos++;
        tokenizer->column++;

        // Read fractional part
        while (tokenizer->pos < tokenizer->length && isdigit(tokenizer->input[tokenizer->pos])) {
            tokenizer->pos++;
            tokenizer->column++;
        }
    }

    int length = tokenizer->pos - start;
    char *number = malloc(length + 1);
    strncpy(number, &tokenizer->input[start], length);
    number[length] = '\0';

    return number;
}

// Helper function to read a string literal
static char *read_string(Tokenizer *tokenizer) {
    int start = tokenizer->pos;

    // Must start with quote
    if (tokenizer->input[tokenizer->pos] != '"') {
        return NULL;
    }

    tokenizer->pos++; // Skip opening quote
    tokenizer->column++;

    // Read until closing quote or end of input
    while (tokenizer->pos < tokenizer->length && tokenizer->input[tokenizer->pos] != '"') {
        if (tokenizer->input[tokenizer->pos] == '\\') {
            // Skip escape sequence
            tokenizer->pos++;
            tokenizer->column++;
            if (tokenizer->pos < tokenizer->length) {
                tokenizer->pos++;
                tokenizer->column++;
            }
        } else if (tokenizer->input[tokenizer->pos] == '\n') {
            tokenizer->pos++;
            tokenizer->line++;
            tokenizer->column = 1;
        } else {
            tokenizer->pos++;
            tokenizer->column++;
        }
    }

    if (tokenizer->pos < tokenizer->length && tokenizer->input[tokenizer->pos] == '"') {
        tokenizer->pos++; // Skip closing quote
        tokenizer->column++;
    }

    // Include the quotes in the returned string
    int length = tokenizer->pos - start;
    char *string = malloc(length + 1);
    strncpy(string, &tokenizer->input[start], length);
    string[length] = '\0';

    return string;
}

// Helper function to check if identifier is a keyword
static int get_keyword_token(const char *identifier) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(identifier, keywords[i].keyword) == 0) {
            return keywords[i].token_type;
        }
    }
    return TEA_TOKEN_IDENT;
}

// Helper function to create a token
static Token create_token(int type, char *value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

// Create a new tokenizer
Tokenizer *tokenizer_create(const char *input) {
    Tokenizer *tokenizer = malloc(sizeof(Tokenizer));
    if (!tokenizer) {
        return NULL;
    }

    tokenizer->input = input;
    tokenizer->pos = 0;
    tokenizer->line = 1;
    tokenizer->column = 1;
    tokenizer->length = strlen(input);
    tokenizer->current_token = create_token(0, NULL, 0, 0);

    return tokenizer;
}

// Free a tokenizer
void tokenizer_free(Tokenizer *tokenizer) {
    if (tokenizer) {
        if (tokenizer->current_token.value) {
            free(tokenizer->current_token.value);
        }
        free(tokenizer);
    }
}

// Check if tokenizer has more tokens
bool tokenizer_has_more(Tokenizer *tokenizer) {
    if (!tokenizer) {
        return false;
    }

    skip_whitespace(tokenizer);
    return tokenizer->pos < tokenizer->length;
}

// Get the next token
Token tokenizer_next_token(Tokenizer *tokenizer) {
    if (!tokenizer) {
        return create_token(-1, NULL, 0, 0);
    }

    skip_whitespace(tokenizer);

    if (tokenizer->pos >= tokenizer->length) {
        return create_token(0, NULL, tokenizer->line, tokenizer->column);
    }

    int line = tokenizer->line;
    int column = tokenizer->column;
    char c = tokenizer->input[tokenizer->pos];

    // Handle single-character tokens
    switch (c) {
        case '@':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_AT, _strdup("@"), line, column);

        case '(':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_LPAREN, _strdup("("), line, column);

        case ')':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_RPAREN, _strdup(")"), line, column);

        case '{':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_LBRACE, _strdup("{"), line, column);

        case '}':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_RBRACE, _strdup("}"), line, column);

        case ':':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_COLON, _strdup(":"), line, column);

        case ',':
            tokenizer->pos++;
            tokenizer->column++;
            return create_token(TEA_TOKEN_COMMA, _strdup(","), line, column);
    }

    // Handle identifiers and keywords
    if (isalpha(c) || c == '_') {
        char *identifier = read_identifier(tokenizer);
        if (identifier) {
            int token_type = get_keyword_token(identifier);
            return create_token(token_type, identifier, line, column);
        }
    }

    // Unknown character - skip it
    tokenizer->pos++;
    tokenizer->column++;
    return create_token(-1, NULL, line, column);
}

// Peek at next token without consuming it
Token tokenizer_peek_token(Tokenizer *tokenizer) {
    // Save current state
    int saved_pos = tokenizer->pos;
    int saved_line = tokenizer->line;
    int saved_column = tokenizer->column;
    Token saved_token = tokenizer->current_token;

    // Get next token
    Token peek_token = tokenizer_next_token(tokenizer);

    // Restore state
    tokenizer->pos = saved_pos;
    tokenizer->line = saved_line;
    tokenizer->column = saved_column;

    // Free the peeked token value if it was allocated
    if (peek_token.value) {
        free(peek_token.value);
    }

    tokenizer->current_token = saved_token;

    // Return a copy with token type only (no value to avoid memory issues)
    Token result;
    result.type = peek_token.type;
    result.value = NULL;
    result.line = peek_token.line;
    result.column = peek_token.column;

    return result;
}
