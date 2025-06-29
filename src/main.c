#include "tea_lang.h"

#include <rtl_log.h>

void print_usage(const char *program_name) {
    rtl_log_i("Usage: %s [options] <tea_file>\n", program_name);
    rtl_log_i("Options:\n");
    rtl_log_i("  -h, --help     Show this help message\n");
    rtl_log_i("  -t, --test     Run with test input\n");
    rtl_log_i("\n");
    rtl_log_i("Examples:\n");
    rtl_log_i("  %s example.tea\n", program_name);
    rtl_log_i("  %s --test\n", program_name);
}

void run_test_parsing() {
    rtl_log_i("Running TEA language parser tests...\n\n");
    
    // Test 1: Simple function with empty body
    rtl_log_i("=== Test 1: Simple Function ===\n");
    const char *test1 = "fn hello() {}";
    rtl_log_i("Input: %s\n", test1);
    
    ASTNode *ast1 = parse_tea_string(test1);
    if (ast1) {
        rtl_log_i("AST:\n");
        print_ast(ast1, 0);
        free_ast(ast1);
    }
    rtl_log_i("\n");
    
    // Test 2: Function with parameters
    rtl_log_i("=== Test 2: Function with Parameters ===\n");
    const char *test2 = "fn add(x: i32, y: i32) {}";
    rtl_log_i("Input: %s\n", test2);
    
    ASTNode *ast2 = parse_tea_string(test2);
    if (ast2) {
        rtl_log_i("AST:\n");
        print_ast(ast2, 0);
        free_ast(ast2);
    }
    rtl_log_i("\n");
    
    // Test 3: Function with attributes
    rtl_log_i("=== Test 3: Function with Attributes ===\n");
    const char *test3 = "@export fn calculate(value: i32) {}";
    rtl_log_i("Input: %s\n", test3);
    
    ASTNode *ast3 = parse_tea_string(test3);
    if (ast3) {
        rtl_log_i("AST:\n");
        print_ast(ast3, 0);
        free_ast(ast3);
    }
    rtl_log_i("\n");
    
    rtl_log_i("Test parsing completed.\n");
}

int main(int argc, char *argv[]) {
    rtl_log_i("TEA Language Parser v1.0 - Simplified\n");
    rtl_log_i("======================================\n\n");
    
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
            rtl_log_e("Unknown option: %s\n", argv[i]);
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
        rtl_log_e("Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if file exists
    FILE *test_file = fopen(filename, "r");
    if (!test_file) {
        rtl_log_e("Error: Cannot open file '%s'\n", filename);
        return 1;
    }
    fclose(test_file);
    
    rtl_log_i("Parsing file: %s\n\n", filename);
    
    // Parse the TEA file
    ASTNode *ast = parse_tea_file(filename);
    
    if (ast) {
        rtl_log_i("\n=== Abstract Syntax Tree ===\n");
        print_ast(ast, 0);
        
        rtl_log_i("\n=== Parsing Summary ===\n");
        rtl_log_i("File: %s\n", filename);
        rtl_log_i("Status: Successfully parsed\n");
        rtl_log_i("Root node type: %s\n", 
               ast->type == NODE_PROGRAM ? "PROGRAM" : "OTHER");
        rtl_log_i("Child nodes: %d\n", ast->child_count);
        
        // Clean up
        free_ast(ast);
        
        rtl_log_i("\nParsing completed successfully!\n");
        return 0;
    } else {
        rtl_log_i("\n=== Parsing Failed ===\n");
        rtl_log_i("File: %s\n", filename);
        rtl_log_i("Status: Failed to parse\n");
        rtl_log_i("Check the error messages above for details.\n");
        
        return 1;
    }
}