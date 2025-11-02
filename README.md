# Tea Programming Language

Tea is a statically-typed scripting language with a focus on memory safety and modern syntax. It combines
the expressiveness and ease-of-use of scripting languages with key design principles for safer code.

## Features

- **Scripting Language**: Designed for automation, quick tasks, and rapid development
- **Memory Safety**: Explicit mutability control with `let` and `mut` keywords
- **Static Typing**: Type annotations with inference capabilities for safer scripts
- **Types and Methods**: Data structures with associated methods via `impl` blocks
- **Control Flow**: Standard control structures (`if`/`else`, `while` loops)

## Influences

- **Explicit Mutability**: Variables are immutable by default (`let`), mutable when specified (`let mut`)
- **Impl Blocks**: Methods are defined separately from type definitions using `impl` blocks
- **Type Annotations**: Optional but explicit type annotations with `:` for safer scripts
- **Arrow Syntax**: Function return types specified with `->`
- **Memory Safety Focus**: Controlled mutability helps prevent common scripting errors

## Syntax Overview

### Variables

Tea uses `let` for variable declarations with optional mutability and optionality:

```text
let x = 42;                   // Immutable variable
let mut y = 10;               // Mutable variable
let z: i32 = 100;             // Explicit type annotation
let mut w: f32 = 3.14;        // Mutable with type
let opt_var: i32? = 5;        // Optional type
let mut opt_mut: f32? = 2.0;  // Optional mutable type
```

**Optional Types**: Types can be marked as optional using the `?` suffix. Optional types can hold a value or be null,
providing safer handling of potentially absent values.

### Functions

Functions are declared with the `fn` keyword:

```text
// Simple function
fn greet() {
    // Function body
}

// Function with parameters and return type
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

// Mutable function (can modify external state)
fn mut increment_counter() -> i32 {
    // Implementation
}
```

### Types

Define custom data types with `typedef`:

```text
typedef Point {
    x: f32;
    y: f32;
}

typedef Person {
    name: string;
    age: i32;
}
```

### Methods

Implement methods for structs using `impl` blocks:

```text
impl Point {
    fn distance(other: Point) -> f32 {
        let dx = self.x - other.x;
        let dy = self.y - other.y;
        return dx * dx + dy * dy;
    }
    
    fn mut move(dx: f32, dy: f32) {
        self.x = self.x + dx;
        self.y = self.y + dy;
    }
}
```

### Instantiation

Create instances with the `new` keyword:

```text
let point = new Point {
    x: 10.0,
    y: 20.0,
};

let person = new Person {
    name: 'Alice',
    age: 30,
};
```

### Control Flow

Standard control structures are supported:

```text
// If-else statements
if x > 0 {
    // Positive
} else {
    // Non-positive
}

// While loops
let mut i = 0;
while i < 10 {
    i = i + 1;
}
```

### Expressions

Tea supports rich expressions with operator precedence:

```text
let result = (a + b) * c - d / e;
let comparison = x > y && z <= w;
let negation = -value;
```

## Data Types

- `i32` - 32-bit signed integer numbers
- `f32` - 32-bit floating-point numbers
- `string` - Text strings
- Custom struct types

## Example Program

```text
typedef Rectangle {
    width: f32;
    height: f32;
}

impl Rectangle {
    fn area() -> f32 {
        return self.width * self.height;
    }
    
    fn mut scale(factor: f32) {
        self.width = self.width * factor;
        self.height = self.height * factor;
    }
}

fn main() {
    let mut rect = new Rectangle {
        width: 10.0,
        height: 5.0,
    };
    
    let initial_area = rect.area();
    rect.scale(2.0);
    let scaled_area = rect.area();
    
    // Example with optional types
    let optional_width?: f32 = 15.0;
    let mut optional_height?: f32 = 8.0;
}
```

## Native Function Binding

Tea supports binding native C functions to make them callable from Tea code. This allows you to extend Tea with system
functionality, I/O operations, and performance-critical code written in C.

### Defining Native Functions

Native functions in C must follow this signature:

```c
tea_val_t your_function_name(tea_fn_args_t* args)
```

The function receives a context and a list of arguments, and must return a `tea_value_t`. Arguments are accessed by
calling `tea_function_args_pop()` in a loop. Here's an example:

```c
// Native function to print values (similar to built-in print)
static tea_val_t tea_print_values(tea_fn_args_t* args) {
    for (;;) {
        tea_var_t* arg = tea_fn_args_pop(args);
        if (!arg) {
            break; // No more arguments
        }
        
        const tea_val_t value = arg->val;
        switch (value.type) {
            case TEA_V_I32:
                printf("%d ", value.i32);
                break;
            case TEA_V_F32:
                printf("%f ", value.f32);
                break;
            case TEA_V_INST:
            case TEA_V_UNDEF:
                break;
        }
    }
    
    return tea_val_undef();
}

// Native function to add two numbers
static tea_val_t tea_add_numbers(tea_fn_args_t* args) {
    tea_var_t* arg1 = tea_fn_args_pop(args);
    tea_var_t* arg2 = tea_fn_args_pop(args);
    
    if (!arg1 || !arg2) {
        // Handle error case - not enough arguments
        return tea_val_undef();
    }
    
    if (arg1->val.type == TEA_V_I32 && arg2->val.type == TEA_V_I32) {
        tea_val_t result = {0};
        result.type = TEA_V_I32;
        result.i32 = arg1->val.i32 + arg2->val.i32;
        return result;
    }
    
    return tea_val_undef();
}
```

### Binding Functions

Register your native functions with the Tea context using `tea_bind_native_fn`:

```c
int main() {
    tea_ctx_t context;
    tea_interp_init(&context, "example.tea");
    
    // Bind native functions
    tea_bind_native_fn(&context, "print", tea_print);
    tea_bind_native_fn(&context, "add_numbers", tea_add_numbers);
    tea_bind_native_fn(&context, "print_values", tea_print_values);
    
    // Execute Tea code...
    
    tea_interp_cleanup(&context);
    return 0;
}
```

### Using Native Functions in Tea

Once bound, native functions can be called like regular Tea functions:

```text
// Call the built-in print function
print('Hello from Tea!');

// Call custom native functions
let sum = add_numbers(10, 20);
print(sum);

// Print multiple values at once
print_values('Result:', sum, 'Done');
```

### Tea Value Types

Native functions work with the `tea_val_t` type system:

- `i32` (TEA_V_I32) - 32-bit signed integers
- `f32` (TEA_V_F32) - 32-bit floating-point numbers
- `string` (TEA_V_INST) - Null-terminated strings wrapped as instances
- `instance` (TEA_V_INST) - Complex objects (structs)
- `undef` (TEA_V_UNDEF) - Uninitialized or error state

### Best Practices

1. **Always validate arguments**: Use `tea_fn_args_pop()` to safely access arguments and check for NULL
2. **Handle errors gracefully**: Return `tea_val_undef()` for error conditions
3. **Memory management**:
    - Strings returned from native functions should be allocated with `tea_malloc`
4. **Performance**: Use native functions for computationally intensive operations
5. **Argument handling**: Process arguments in the order they were passed by calling `tea_fn_args_pop()` sequentially

## Building

Tea uses CMake for building:

```bash
# Configure and build in one go
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Or for development builds
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

## Language Status

Tea is currently in development. The core language features are implemented including:

- ✅ Variable declarations and assignments
- ✅ Optional types with `?` syntax
- ✅ Function definitions and calls
- ✅ Struct definitions and instantiation
- ✅ Method definitions with impl blocks
- ✅ Control flow statements
- ✅ Expression evaluation
- ✅ Type system foundations
- ✅ Native function binding

**Planned Features:**

- 🔄 Advanced type inference
- 🔄 Module system
