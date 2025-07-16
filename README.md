# Tea Programming Language

Tea is a Rust-inspired, statically-typed scripting language with a focus on memory safety and modern syntax. It combines
the expressiveness and ease-of-use of scripting languages with Rust's key design principles for safer code.

## Features

- **Scripting Language**: Designed for automation, quick tasks, and rapid development
- **Rust-Inspired Design**: Familiar syntax and concepts for Rust developers
- **Memory Safety**: Explicit mutability control with `let` and `mut` keywords
- **Static Typing**: Type annotations with inference capabilities for safer scripts
- **Structs and Methods**: Data structures with associated methods via `impl` blocks
- **Control Flow**: Standard control structures (`if`/`else`, `while` loops)
- **Traits (Planned)**: Define shared behavior across types with trait definitions

## Rust Influences

Tea brings Rust's safety concepts to scripting:

- **Explicit Mutability**: Variables are immutable by default (`let`), mutable when specified (`let mut`)
- **Impl Blocks**: Methods are defined separately from struct definitions using `impl` blocks
- **Type Annotations**: Optional but explicit type annotations with `:` for safer scripts
- **Arrow Syntax**: Function return types specified with `->`
- **Traits (Planned)**: Rust-style trait system for defining shared behavior
- **Memory Safety Focus**: Controlled mutability helps prevent common scripting errors

## Syntax Overview

### Variables

Tea uses `let` for variable declarations with optional mutability:

```tea
let x = 42;              // Immutable variable
let mut y = 10;          // Mutable variable
let z: i32 = 100;        // Explicit type annotation
let mut w: f32 = 3.14;   // Mutable with type
```

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

### Structs

Define custom data types with `struct`:

```tea
struct Point {
    x: f32;
    y: f32;
}

struct Person {
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

### Traits (Planned)

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
}
```

## Native Function Binding

Tea supports binding native C functions to make them callable from Tea code. This allows you to extend Tea with system
functionality, I/O operations, and performance-critical code written in C.

### Defining Native Functions

Native functions in C must follow this signature:

```c
tea_value_t your_function_name(const tea_value_t* args, int arg_count)
```

The function receives an array of `tea_value_t` arguments and must return a `tea_value_t`. Here's an example:

```c
// Native function to add two numbers
static tea_value_t tea_add_numbers(const tea_value_t* args, const int arg_count) {
    if (arg_count != 2) {
        // Handle error case
        return tea_value_unset();
    }
    
    if (args[0].type == TEA_VALUE_I32 && args[1].type == TEA_VALUE_I32) {
        tea_value_t result = {0};
        result.type = TEA_VALUE_I32;
        result.i32_value = args[0].i32_value + args[1].i32_value;
        return result;
    }
    
    return tea_value_unset();
}

// Native function for file operations
static tea_value_t tea_read_file(const tea_value_t* args, const int arg_count) {
    if (arg_count != 1 || args[0].type != TEA_VALUE_STRING) {
        return tea_value_unset();
    }
    
    FILE* file = fopen(args[0].string_value, "r");
    if (!file) {
        return tea_value_unset();
    }
    
    // Read file content...
    // (implementation details omitted for brevity)
    
    tea_value_t result = {0};
    result.type = TEA_VALUE_STRING;
    result.string_value = file_content; // allocated string
    return result;
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
    tea_bind_native_function(&context, "read_file", tea_read_file);
    
    // Execute Tea code...
    
    tea_interpret_cleanup(&context);
    return 0;
}
```

### Using Native Functions in Tea

Once bound, native functions can be called like regular Tea functions:

```tea
// Call the built-in print function
print("Hello from Tea!");

// Call custom native functions
let sum = add_numbers(10, 20);
print(sum);

// File operations
let content = read_file("data.txt");
print(content);
```

### Tea Value Types

Native functions work with the `tea_value_t` type system:

- `TEA_VALUE_I32` - 32-bit signed integers
- `TEA_VALUE_F32` - 32-bit floating-point numbers
- `TEA_VALUE_STRING` - Null-terminated strings
- `TEA_VALUE_OBJECT` - Complex objects (structs)
- `TEA_VALUE_UNSET` - Uninitialized or error state

### Best Practices

1. **Always validate arguments**: Check argument count and types
2. **Handle errors gracefully**: Return `tea_value_unset()` if there is nothing to return
3. **Memory management**: Strings returned from native functions should be allocated with `rtl_malloc`
4. **Performance**: Use native functions for computationally intensive operations

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
- ✅ Function definitions and calls
- ✅ Struct definitions and instantiation
- ✅ Method definitions with impl blocks
- ✅ Control flow statements
- ✅ Expression evaluation
- ✅ Type system foundations
- ✅ Native function binding

**Planned Features:**

- 🔄 Trait system for shared behavior
- 🔄 Advanced type inference
- 🔄 Module system
