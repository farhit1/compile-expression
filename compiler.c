#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>


#define MAX_ARGS 4
#define MAX_LENGTH 100

typedef struct {
    const char * name;
    void * pointer;
} symbol_t;

typedef enum {
    EXPR_SUM,               // binary_expression_t
    EXPR_SUB,               // binary_expression_t
    EXPR_MUL,               // binary_expression_t
    EXPR_EXTERN_VALUE,      // extern_value_t
    EXPR_EXTERN_FUNCTION,   // extern_function_t
    EXPR_VALUE,             // value_t
    EXPR_BRACED_SUBEXPR     // braced_subexpression_t
} node_type_t;

const uint32_t ALWAYS = 0xE0000000;

typedef enum {
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    R5 = 5,
    R6 = 6,
    R7 = 7,
    SP = 13,
    LR = 14
} registry_t;

////////////////////////
///                  ///
///   DECLARATIONS   ///
///                  ///
////////////////////////

typedef struct node node_t;
node_t * create_node(node_type_t type, ...);
void delete_node(node_t *);

typedef struct binary_expression binary_expression_t;
binary_expression_t * create_binary_expression(node_t * lhs, node_t * rhs);
void delete_binary_expression(binary_expression_t *);

typedef struct extern_value extern_value_t;
extern_value_t * create_extern_value(void * value_ptr);
void delete_extern_value(extern_value_t *);

typedef struct extern_function extern_function_t;
extern_function_t * create_extern_function(void * function_ptr, size_t args_n, node_t ** args);
void delete_extern_function(extern_function_t *);

typedef struct value value_t;
value_t * create_value(int value);
void delete_value(value_t *);

typedef struct braced_subexpression braced_subexpression_t;
braced_subexpression_t * create_braced_subexpression(node_t * subexpression);
void delete_braced_subexpression(braced_subexpression_t *);

node_t * fetch(size_t * position, const char * expression, const symbol_t * externs);
node_t * fetch_extern(size_t * position, const char * expression, const symbol_t * externs);
node_t * fetch_extern_function(void * function_ptr, size_t * position,
                               const char * expression, const symbol_t * externs);
uint32_t fetch_value(size_t * position, const char * expression);

typedef uint32_t instruction_t;

instruction_t push(uint32_t mask);
instruction_t pop(uint32_t mask);
instruction_t mov(registry_t dst, registry_t src);
instruction_t ldr(registry_t dst, registry_t src);
instruction_t add(registry_t dst, registry_t src);
instruction_t sub(registry_t dst, registry_t src);
instruction_t mul(registry_t dst, registry_t src);
instruction_t default_push();
instruction_t default_pop();

void write_instruction(instruction_t, instruction_t ** out_buffer_ptr);
void mov_value(registry_t dst, uint32_t value, instruction_t ** out_buffer_ptr);

void compile(node_t *, instruction_t ** out_buffer_ptr);
void compile_binary(node_type_t type, binary_expression_t *, instruction_t ** out_buffer_ptr);
void compile_extern_value(extern_value_t *, instruction_t ** out_buffer_ptr);
void compile_extern_function(extern_function_t *, instruction_t ** out_buffer_ptr);
void compile_value(value_t *, instruction_t ** out_buffer_ptr);
void compile_braced_subexpression(braced_subexpression_t *, instruction_t ** out_buffer_ptr);

const char *
remove_spaces(const char * expression);


///////////////////////
///                 ///
///   DEFINITIONS   ///
///                 ///
///////////////////////

struct node {
    node_type_t type;
    void * expression;
};

node_t *
create_node(node_type_t type, ...)
{
    node_t * node = malloc(sizeof(node_t));
    node->type = type;

    va_list args;
    switch (type) {
        case EXPR_SUM:
        case EXPR_SUB:
        case EXPR_MUL:
            va_start(args, 2);
            node->expression = create_binary_expression(va_arg(args, node_t *),
                                                        va_arg(args, node_t *));
            break;
        case EXPR_EXTERN_VALUE:
            va_start(args, 1);
            node->expression = create_extern_value(va_arg(args, void *));
            break;
        case EXPR_EXTERN_FUNCTION:
            va_start(args, 3);
            node->expression = create_extern_function(va_arg(args, void *),
                                                      va_arg(args, size_t),
                                                      va_arg(args, node_t **));
            break;
        case EXPR_VALUE:
            va_start(args, 1);
            node->expression = create_value(va_arg(args, int));
            break;
        case EXPR_BRACED_SUBEXPR:
            va_start(args, 1);
            node->expression = create_braced_subexpression(va_arg(args, node_t *));
            break;
    }
    va_end(args);

    return node;
}

void
delete_node(node_t * node)
{
    switch (node->type) {
        case EXPR_SUM:
        case EXPR_SUB:
        case EXPR_MUL:
            delete_binary_expression(node->expression);
            break;
        case EXPR_EXTERN_VALUE:
            delete_extern_value(node->expression);
            break;
        case EXPR_EXTERN_FUNCTION:
            delete_extern_function(node->expression);
            break;
        case EXPR_VALUE:
            delete_value(node->expression);
            break;
        case EXPR_BRACED_SUBEXPR:
            delete_braced_subexpression(node->expression);
            break;
    }

    free(node);
}


struct binary_expression {
    node_t * lhs;
    node_t * rhs;
};

binary_expression_t *
create_binary_expression(node_t * lhs,
                         node_t * rhs)
{
    binary_expression_t * binary_expression = malloc(sizeof(binary_expression_t));
    binary_expression->lhs = lhs;
    binary_expression->rhs = rhs;
    return binary_expression;
}

void
delete_binary_expression(binary_expression_t * binary_expression)
{
    delete_node(binary_expression->lhs);
    delete_node(binary_expression->rhs);
    free(binary_expression);
}


struct extern_value {
    void * value_ptr;
};

extern_value_t *
create_extern_value(void * value_ptr)
{
    extern_value_t * extern_value = malloc(sizeof(extern_value_t));
    extern_value->value_ptr = value_ptr;
    return extern_value;
}

void
delete_extern_value(extern_value_t * extern_value)
{
    free(extern_value);
}


struct extern_function {
    void * function_ptr;
    size_t args_n;
    node_t ** args;
};

extern_function_t *
create_extern_function(void * function_ptr,
                       size_t args_n,
                       node_t ** args)
{
    extern_function_t * extern_function = malloc(sizeof(extern_function_t));
    extern_function->function_ptr = function_ptr;
    extern_function->args_n = args_n;
    extern_function->args = args;
    return extern_function;
}

void
delete_extern_function(extern_function_t * extern_function)
{
    for (size_t i = 0; i < extern_function->args_n; i++) {
        delete_node(extern_function->args[i]);
    }
    free(extern_function->args);
    free(extern_function);
}


struct value {
    int val;
};

value_t *
create_value(int value)
{
    value_t * val = malloc(sizeof(value_t));
    val->val = value;
    return val;
}

void
delete_value(value_t * value)
{
    free(value);
}


struct braced_subexpression {
    node_t * subexpression;
};

braced_subexpression_t *
create_braced_subexpression(node_t * subexpression)
{
    braced_subexpression_t * braced_subexpression = malloc(sizeof(subexpression));
    braced_subexpression->subexpression = subexpression;
    return braced_subexpression;
}

void
delete_braced_subexpression(braced_subexpression_t * braced_expression)
{
    delete_node(braced_expression->subexpression);
    free(braced_expression);
}


node_t *
fetch(size_t *position,
      const char *expression,
      const symbol_t *externs)
{
    node_t * prev_addend = NULL;
    node_t * prev_factor = NULL;
    char last_addend_binary_operation = 0;
    char last_binary_operation = 0;

    while (1) {
        int is_parse_end = 0;
        node_t * operation = NULL;

        char symbol = expression[*position];
        switch (symbol) {
            case 0:
            case ')':
            case ',':
                is_parse_end = 1;
                break;

            case '+':
            case '-':
            case '*':
                last_binary_operation = symbol;
                (*position)++;
                break;

            case '(':
                (*position)++;
                operation = create_node(EXPR_BRACED_SUBEXPR,
                                        fetch(position, expression, externs));
                (*position)++;
                break;

            default:
                if (isdigit(symbol)) {
                    operation = create_node(EXPR_VALUE,
                                            fetch_value(position, expression));
                }
                else {
                    operation = fetch_extern(position, expression, externs);
                }
                break;
        }

        // merge prev_addend and prev_factor
        if (is_parse_end) {
            last_binary_operation = '+';
        }

        if (!operation && !is_parse_end) {
            continue;
        }

        switch (last_binary_operation) {
            case 0:
                prev_factor = operation;
                break;

            case '+':
            case '-':
                // handle unary minus
                if (!prev_factor) {
                    prev_factor = create_node(EXPR_VALUE, -1);
                }
                else {
                    if (last_addend_binary_operation) {
                        prev_addend = create_node(last_addend_binary_operation == '+' ? EXPR_SUM : EXPR_SUB,
                                                  prev_addend, prev_factor);
                    }
                    else {
                        prev_addend = prev_factor;
                    }
                    prev_factor = operation;
                    last_addend_binary_operation = last_binary_operation;
                    break;
                }
            case '*':
                prev_factor = create_node(EXPR_MUL, prev_factor, operation);
                break;
        }

        if (is_parse_end) {
            break;
        }
    }

    return prev_addend;
}

node_t *
fetch_extern(size_t * position,
             const char * expression,
             const symbol_t * externs)
{
    char extern_name[MAX_LENGTH];

    size_t extern_name_length = 0;
    while (isalnum(expression[*position])) {
        extern_name[extern_name_length] = expression[*position];
        extern_name_length++;
        (*position)++;
    }
    extern_name[extern_name_length] = 0;

    void * extern_ptr;
    for (size_t i = 0; ; i++) {
        if (strcmp(extern_name, externs[i].name) == 0) {
            extern_ptr = externs[i].pointer;
            break;
        }
    }

    if (expression[*position] == '(') {
        (*position)++;
        return fetch_extern_function(extern_ptr, position, expression, externs);
    }
    return create_node(EXPR_EXTERN_VALUE, extern_ptr);
}

node_t *
fetch_extern_function(void * function_ptr,
                      size_t * position,
                      const char * expression,
                      const symbol_t * externs)
{
    size_t args_n = 0;
    node_t ** args = calloc(MAX_ARGS, sizeof(node_t *));

    while (1) {
        args[args_n++] = fetch(position, expression, externs);
        if (expression[*position] == ')') {
            (*position)++;
            break;
        }
        (*position)++;
    }

    return create_node(EXPR_EXTERN_FUNCTION, function_ptr, args_n, args);
}

uint32_t
fetch_value(size_t * position,
            const char * expression)
{
    uint32_t number = 0;
    while (isdigit(expression[*position])) {
        number *= 10;
        number += expression[*position] - '0';
        (*position)++;
    }
    return number;
}


instruction_t
push(uint32_t mask)
{
    return ALWAYS | (1 << 27) | (1 << 24) | (1 << 21) | (SP << 16) | mask;
}

instruction_t
pop(uint32_t mask)
{
    return ALWAYS | (1 << 27) | (1 << 23) | (1 << 21) | (1 << 20) | (SP << 16) | mask;
}

instruction_t
mov(registry_t dst,
    registry_t src)
{
    return ALWAYS | (1 << 24) | (1 << 23) | (1 << 21) | (dst << 12) | src;
}

instruction_t
ldr(registry_t dst,
    registry_t src)
{
    return ALWAYS | (1 << 26) | (1 << 20) | (dst << 16) | (dst << 12);
}

instruction_t
add(registry_t dst,
    registry_t src)
{
    return ALWAYS | (1 << 23) | (dst << 16) | (dst << 12) | src;
}

instruction_t
sub(registry_t dst,
    registry_t src)
{
    return ALWAYS | (1 << 22) | (dst << 16) | (dst << 12) | src;
}

instruction_t
mul(registry_t dst,
    registry_t src)
{
    return ALWAYS | (1 << 7) | (1 << 4) | (dst << 16) | (dst << 8) | src;
}

instruction_t
default_push()
{
    return push((1 << R4) | (1 << R5) | (1 << R6) | (1 << R7) | (1 << LR));
}

instruction_t
default_pop()
{
    return pop((1 << R4) | (1 << R5) | (1 << R6) | (1 << R7) | (1 << LR));
}


void
write_instruction(instruction_t instruction,
                  instruction_t ** out_buffer_ptr)
{
    **out_buffer_ptr = instruction;
    (*out_buffer_ptr)++;
}

void
mov_value(registry_t dst,
          uint32_t value,
          instruction_t ** out_buffer_ptr)
{
    write_instruction(ALWAYS | (1 << 25) | (1 << 24) | (1 << 23) | (1 << 21) | (dst << 12), out_buffer_ptr);
    for (uint32_t mask = 0xFF, rt = 16, shift = 0, it = 0;
         it < 4;
         mask <<= (uint32_t) 8, rt -= 4, shift += 8, it++)
    {
        uint32_t val = (value & mask) >> shift;
        uint32_t rot = rt % 16;
        write_instruction(ALWAYS | (1 << 25) | (1 << 24) | (1 << 23)
                          | (dst << 16) | (dst << 12) | (rot << 8) | val, out_buffer_ptr);
    }
}


void
compile(node_t * node,
        instruction_t ** out_buffer_ptr)
{
    write_instruction(default_push(), out_buffer_ptr);
    switch (node->type) {
        case EXPR_SUM:
        case EXPR_SUB:
        case EXPR_MUL:
            compile_binary(node->type, node->expression, out_buffer_ptr);
            break;
        case EXPR_EXTERN_VALUE:
            compile_extern_value(node->expression, out_buffer_ptr);
            break;
        case EXPR_EXTERN_FUNCTION:
            compile_extern_function(node->expression, out_buffer_ptr);
            break;
        case EXPR_VALUE:
            compile_value(node->expression, out_buffer_ptr);
            break;
        case EXPR_BRACED_SUBEXPR:
            compile_braced_subexpression(node->expression, out_buffer_ptr);
            break;
    }
    write_instruction(default_pop(), out_buffer_ptr);
}

void
compile_binary(node_type_t type,
               binary_expression_t * binary_expression,
               instruction_t ** out_buffer_ptr)
{
    compile(binary_expression->rhs, out_buffer_ptr);
    write_instruction(mov(R4, R0), out_buffer_ptr);
    compile(binary_expression->lhs, out_buffer_ptr);
    write_instruction(mov(R1, R4), out_buffer_ptr);
    switch (type) {
        case EXPR_SUM:
            write_instruction(add(R0, R1), out_buffer_ptr);
            break;
        case EXPR_SUB:
            write_instruction(sub(R0, R1), out_buffer_ptr);
            break;
        case EXPR_MUL:
            write_instruction(mul(R0, R1), out_buffer_ptr);
            break;
    }
}

void
compile_extern_value(extern_value_t * extern_value,
                     instruction_t ** out_buffer_ptr)
{
    mov_value(R0, (uint32_t) extern_value->value_ptr, out_buffer_ptr);
    write_instruction(ldr(R0, R0), out_buffer_ptr);
}

void
compile_extern_function(extern_function_t * extern_function,
                        instruction_t ** out_buffer_ptr)
{
    for (size_t i = 0; i < extern_function->args_n; i++) {
        compile(extern_function->args[i], out_buffer_ptr);
        write_instruction(mov((registry_t) 4 + i, R0), out_buffer_ptr);
    }
    for (size_t i = 0; i < extern_function->args_n; i++) {
        write_instruction(mov((registry_t) i, (registry_t) 4 + i), out_buffer_ptr);
    }
    mov_value(R4, (uint32_t) extern_function->function_ptr, out_buffer_ptr);
    write_instruction(push(1 << LR), out_buffer_ptr);
    write_instruction(ALWAYS | (1 << 24) | (1 << 21) | (0xFFF << 8) | (1 << 5) | (1 << 4) | R4, out_buffer_ptr);
    write_instruction(pop(1 << LR), out_buffer_ptr);
}

void
compile_value(value_t * value,
              instruction_t ** out_buffer_ptr)
{
    mov_value(R0, (uint32_t) value->val, out_buffer_ptr);
}

void
compile_braced_subexpression(braced_subexpression_t * value,
                             instruction_t ** out_buffer_ptr)
{
    compile(value->subexpression, out_buffer_ptr);
}


const char *
remove_spaces(const char * expression)
{
    size_t pos = 0;
    while (expression[pos] != 0) {
        pos++;
    }
    char * spaceless_expression = malloc(pos * sizeof(char) + 1);

    pos = 0;
    size_t spaceless_pos = 0;
    while (expression[pos] != 0) {
        if (expression[pos] != ' ') {
            spaceless_expression[spaceless_pos++] = expression[pos];
        }
        pos++;
    }
    spaceless_expression[pos] = 0;

    return spaceless_expression;
}


void
jit_compile_expression_to_arm(const char * expression,
                              const symbol_t * externs,
                              void * out_buffer)
{
    const char * spaceless_expression = remove_spaces(expression);
    size_t position = 0;
    node_t * tree = fetch(&position, spaceless_expression, externs);
    free(spaceless_expression);

    uint32_t * to = out_buffer;
    compile(tree, &to);
    write_instruction(ALWAYS | 0x12FFF1E, &to);  // bx lr

    delete_node(tree);
}
