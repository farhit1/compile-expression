# JIT expression compilation

```c
typedef struct {
    const char * name;
    void       * pointer;
} symbol_t;

extern void
jit_compile_expression_to_arm(const char * expression,
                              const symbol_t * externs,
                              void * out_buffer);
```

Implement `jit_compile_expression_to_arm`.

Fill `out_buffer` with program code for ARM-processor. Expression may contain:
- integers,
- names of global variables,
- names of function, returning an integer and having from 0 to 4 integer parameters,
- addition, subtraction and multiplication operators,
- braced subexpressions.

`externs` contains an array of external symbols (both functions and variables).

`main.c` contains testing program source.

#### Sample

**Input**
```
.vars
a=1 b=2 c=3
.expression
(1+a)*c + div(2+4,2)
```
**Output**
```
9
```