The input to the front-end is a SQL query. the output is sqlite virtual machine bytecode (essentially a compiled program that can operate on the database).

front-end consists of the:

tokenizer
parser
code generator

back-end consists of the:

virtual machine(takes the byte code to get instruction like switch statement)
B-tree(stores the table)
pager(reading and writing the correct page)
os interface

Phase 1:
REPL Interface

#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>
typedef struct
{
char \*buffer;
size_t buffer_length;
ssize_t input_length;
} InputBuffer;

InputBuffer *new_input_buffer()
{
InputBuffer *input_buffer = (InputBuffer \*)malloc(sizeof(InputBuffer));
input_buffer->buffer = NULL;
input_buffer->buffer_length = 0;
input_buffer->input_length = 0;

    return input_buffer;

}
////
i made a structure and a value initialization for the structure which will hold the input buffer.
Consist of buffer, buffer_length, input_length .
Here buffer is the the memory.

Notes:
-size_t is an unsigned integer data type used to represent the size of an object in memory or the result of the sizeof operator.
-ssize_t is a signed integer version of size_t
-\* symbol is used to indicate a pointer. A pointer is a variable that stores the memory address of another variable.
////
