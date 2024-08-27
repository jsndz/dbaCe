25/08/2024
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

`#include <stdio.h>
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
`

////
i made a structure and a value initialization for the structure which will hold the input buffer.
Consist of buffer, buffer_length, input_length .
Here buffer is the the memory.

Notes:
-size_t is an unsigned integer data type used to represent the size of an object in memory or the result of the sizeof operator.
-ssize_t is a signed integer version of size_t
-\* symbol is used to indicate a pointer. A pointer is a variable that stores the memory address of another variable.
////

27/08/20024

`void print_prompt()
{
printf("db>");
}

void read_input(InputBuffer \*inputBuffer)
{
ssize_t bytes_read = getline(&(inputBuffer->buffer), &(inputBuffer->buffer_length), stdin);
if (bytes_read <= 0)
{
printf("error in reading\n");
exit(EXIT_FAILURE);
}
inputBuffer->buffer[bytes_read - 1] = 0;
inputBuffer->input_length = bytes_read - 1;
}

void close_input_buffer(InputBuffer \*inputBuffer)
{
free(inputBuffer->buffer);
free(inputBuffer);
}

int main(int argc, int *argv)
{
InputBuffer *inputBuffer = new_input_buffer();
while (1)
{
print_prompt();
read_input(inputBuffer);
if (strcmp(inputBuffer->buffer, ".exit") == 0)
{
close_input_buffer(inputBuffer);
exit(EXIT_SUCCESS);
}
else
{
printf("Unrecognisable command '%s'", inputBuffer->buffer);
}
}
}`

TO imitate the database interface make print_prompt() function, inside will be the first lines of what you want as a the prompt.
There are two functions here read_input and close_input_buffer
Lets start with read_input():
This reads the input buffer like the commands you enter.
getline() function is used to get the size of the buffer to bytes_read and putting the command in input_buffer.
getline() function:
`getline` reads a line of input and stores it in a buffer. If `*lineptr` is `NULL`, it allocates a buffer and updates `*n` with its size. If `*lineptr` is not `NULL`, it uses the provided buffer, resizing if needed. It returns the number of characters read, including the newline but not the null terminator. If an error occurs or end-of-file is reached, it returns `-1`.

the buffer which stores the command's last character is changed to 0 from "null character"
the input_length is decreased by 1

close_input_buffer(): deallocates memory from input buffer

Notes:
-buffer: A place (a memory buffer) where the input line is stored.
-buffer_length: The size of the buffer.
-input_length: The number of characters actually read from the input.
-The getline function in C is used to read an entire line of text from a stream, typically from standard input
-bytes_read holds the number of characters that getline has read from the input, including the newline character
