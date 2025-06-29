#include "tea_lang.h"

void print_usage(const char *program_name) {
    printf("Usage: %s [options] <tea_file>\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -t, --test     Run with test input\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s example.tea\n", program_name);
    printf("  %s --test\n", program_name);
}

void run_test_parsing() {
    printf("Running TEA language parser tests...\n\n");
    
    // Test 1: Simple function with empty body
    printf("=== Test 1: Simple Function ===\n");
    const char *test1 = "fn hello() {}";
    printf("Input: %s\n", test1);
    
    ASTNode *ast1 = parse_tea_string(test1);
    if (ast1) {
        printf("AST:\n");
        print_ast(ast1, 0);
        free_ast(ast1);
    }
    printf("\n");
    
    // Test 2: Function with parameters
    printf("=== Test 2: Function with Parameters ===\n");
    const char *test2 = "fn add(x: i32, y: i32) {}";
    printf("Input: %s\n", test2);
    
    ASTNode *ast2 = parse_tea_string(test2);
    if (ast2) {
        printf("AST:\n");
        print_ast(ast2, 0);
        free_ast(ast2);
    }
    printf("\n");
    
    // Test 3: Function with attributes
    printf("=== Test 3: Function with Attributes ===\n");
    const char *test3 = "@export fn calculate(value: i32) {}";
    printf("Input: %s\n", test3);
    
    ASTNode *ast3 = parse_tea_string(test3);
    if (ast3) {
        printf("AST:\n");
        print_ast(ast3, 0);
        free_ast(ast3);
    }
    printf("\n");
    
    printf("Test parsing completed.\n");
}

int main(int argc, char *argv[]) {
    printf("TEA Language Parser v1.0 - Simplified\n");
    printf("======================================\n\n");
    
    bool run_tests = false;
    const char *filename = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            run_tests = true;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Run tests if requested
    if (run_tests) {
        run_test_parsing();
        return 0;
    }
    
    // Check if filename was provided
    if (!filename) {
        fprintf(stderr, "Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if file exists
    FILE *test_file = fopen(filename, "r");
    if (!test_file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return 1;
    }
    fclose(test_file);
    
    printf("Parsing file: %s\n\n", filename);
    
    // Parse the TEA file
    ASTNode *ast = parse_tea_file(filename);
    
    if (ast) {
        printf("\n=== Abstract Syntax Tree ===\n");
        print_ast(ast, 0);
        
        printf("\n=== Parsing Summary ===\n");
        printf("File: %s\n", filename);
        printf("Status: Successfully parsed\n");
        printf("Root node type: %s\n", 
               ast->type == NODE_PROGRAM ? "PROGRAM" : "OTHER");
        printf("Child nodes: %d\n", ast->child_count);
        
        // Clean up
        free_ast(ast);
        
        printf("\nParsing completed successfully!\n");
        return 0;
    } else {
        printf("\n=== Parsing Failed ===\n");
        printf("File: %s\n", filename);
        printf("Status: Failed to parse\n");
        printf("Check the error messages above for details.\n");
        
        return 1;
    }
}