# Tea Programming Language

Tea is a Rust-inspired, statically-typed scripting language with a focus on memory safety and modern syntax. It combines
the expressiveness and ease-of-use of scripting languages with Rust's key design principles for safer code.

## Features

- **Scripting Language**: Designed for automation, quick tasks, and rapid development
- **Rust-Inspired Design**: Familiar syntax and concepts for Rust developers
- **Memory Safety**: Explicit mutability control with `let` and `mut` keywords
- **Static Typing**: Type annotations with inference capabilities for safer scripts
- **Types and Methods**: Data structures with associated methods via `impl` blocks
- **Control Flow**: Standard control structures (`if`/`else`, `while` loops)
- **Traits**: Define shared behavior across types with trait definitions

## Rust Influences

Tea brings Rust's safety concepts to scripting:

- **Explicit Mutability**: Variables are immutable by default (`let`), mutable when specified (`let mut`)
- **Impl Blocks**: Methods are defined separately from type definitions using `impl` blocks
- **Type Annotations**: Optional but explicit type annotations with `:` for safer scripts
- **Arrow Syntax**: Function return types specified with `->`
- **Traits**: Rust-style trait system for defining shared behavior
- **Memory Safety Focus**: Controlled mutability helps prevent common scripting errors

## Syntax Overview

### Variables

Tea uses `let` for variable declarations with optional mutability and optionality:

```tea
let x = 42;              // Immutable variable
let mut y = 10;          // Mutable variable
let z: i32 = 100;        // Explicit type annotation
let mut w: f32 = 3.14;   // Mutable with type
let opt_var: i32? = 5;   // Optional type
let mut opt_mut: f32? = 2.0; // Optional mutable type
```

**Optional Types**: Types can be marked as optional using the `?` suffix. Optional types can hold a value or be null, providing safer handling of potentially absent values.

### Functions

Functions are declared with the `fn` keyword:

```tea
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

Define custom data types with `type`:

```tea
type Point {
    x: f32;
    y: f32;
}

type Person {
    name: string;
    age: i32;
}
```

### Methods

Implement methods for structs using `impl` blocks:

```tea
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

### Struct Instantiation

Create struct instances with the `new` keyword:

```tea
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

```tea
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

```tea
let result = (a + b) * c - d / e;
let comparison = x > y && z <= w;
let negation = -value;
```

### Traits

Define shared behavior across types with traits:

```tea
trait Drawable {
    fn draw();
    fn area() -> f32;
}

trait Movable {
    fn mut move_to(x: f32, y: f32);
    fn get_position() -> Point;
}
```

Implement traits for structs:

```tea
impl Drawable for Rectangle {
    fn draw() {
        // Draw rectangle implementation
    }
    
    fn area() -> f32 {
        return self.width * self.height;
    }
}

impl Movable for Rectangle {
    fn mut move_to(x: f32, y: f32) {
        self.x = x;
        self.y = y;
    }
    
    fn get_position() -> Point {
        return new Point { x: self.x, y: self.y };
    }
}
```

## Data Types

- `i32` - 32-bit signed integer numbers
- `f32` - 32-bit floating-point numbers
- `string` - Text strings
- Custom struct types

## Example Program

```tea
struct Rectangle {
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
tea_value_t your_function_name(tea_context_t* context, const tea_function_args_t* args)
```

The function receives a context and a list of arguments, and must return a `tea_value_t`. Arguments are accessed by calling `tea_function_args_pop()` in a loop. Here's an example:

```c
// Native function to print values (similar to built-in print)
static tea_value_t tea_print_values(tea_context_t* context, const tea_function_args_t* args) {
    for (;;) {
        tea_variable_t* arg = tea_function_args_pop(args);
        if (!arg) {
            break; // No more arguments
        }
        
        const tea_value_t value = arg->value;
        switch (value.type) {
            case TEA_VALUE_I32:
                printf("%d ", value.i32);
                break;
            case TEA_VALUE_F32:
                printf("%f ", value.f32);
                break;
            case TEA_VALUE_STRING:
                printf("%s ", value.string);
                break;
            case TEA_VALUE_INSTANCE:
            case TEA_VALUE_INVALID:
                break;
        }
        
        tea_free_variable(context, arg);
    }
    printf("\n");
    
    return tea_value_invalid();
}

// Native function to add two numbers
static tea_value_t tea_add_numbers(tea_context_t* context, const tea_function_args_t* args) {
    tea_variable_t* arg1 = tea_function_args_pop(args);
    tea_variable_t* arg2 = tea_function_args_pop(args);
    
    if (!arg1 || !arg2) {
        // Handle error case - not enough arguments
        if (arg1) tea_free_variable(context, arg1);
        if (arg2) tea_free_variable(context, arg2);
        return tea_value_invalid();
    }
    
    if (arg1->value.type == TEA_VALUE_I32 && arg2->value.type == TEA_VALUE_I32) {
        tea_value_t result = {0};
        result.type = TEA_VALUE_I32;
        result.i32 = arg1->value.i32 + arg2->value.i32;
        
        tea_free_variable(context, arg1);
        tea_free_variable(context, arg2);
        return result;
    }
    
    tea_free_variable(context, arg1);
    tea_free_variable(context, arg2);
    return tea_value_invalid();
}
```

### Binding Functions

Register your native functions with the Tea context using `tea_bind_native_function`:

```c
int main() {
    tea_context_t context;
    tea_interpret_init(&context, "example.tea");
    
    // Bind native functions
    tea_bind_native_function(&context, "print", tea_print);
    tea_bind_native_function(&context, "add_numbers", tea_add_numbers);
    tea_bind_native_function(&context, "print_values", tea_print_values);
    
    // Execute Tea code...
    
    tea_interpret_cleanup(&context);
    return 0;
}
```

### Using Native Functions in Tea

Once bound, native functions can be called like regular Tea functions:

```tea
// Call the built-in print function
print('Hello from Tea!');

// Call custom native functions
let sum = add_numbers(10, 20);
print(sum);

// Print multiple values at once
print_values('Result:', sum, 'Done');
```

### Tea Value Types

Native functions work with the `tea_value_t` type system:

- `TEA_VALUE_I32` - 32-bit signed integers
- `TEA_VALUE_F32` - 32-bit floating-point numbers
- `TEA_VALUE_STRING` - Null-terminated strings
- `TEA_VALUE_INSTANCE` - Complex objects (structs)
- `TEA_VALUE_INVALID` - Uninitialized or error state

### Best Practices

1. **Always validate arguments**: Use `tea_function_args_pop()` to safely access arguments and check for NULL
2. **Handle errors gracefully**: Return `tea_value_invalid()` for error conditions
3. **Memory management**: 
   - Always call `tea_free_variable(context, arg)` for each argument you pop
   - Strings returned from native functions should be allocated with `rtl_malloc`
4. **Performance**: Use native functions for computationally intensive operations
5. **Argument handling**: Process arguments in the order they were passed by calling `tea_function_args_pop()` sequentially

## Building

Tea uses CMake for building. Here are several approaches:

### Modern CMake (Recommended)

```bash
# Configure and build in one go
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Or for development builds
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

### With Ninja (Faster builds)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Traditional approach

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
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
- ✅ Trait system for shared behavior

**Planned Features:**
- 🔄 Advanced type inference
- 🔄 Module system
