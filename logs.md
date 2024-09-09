## 25/08/2024

The input to the front-end is a SQL query. The output is SQLite virtual machine bytecode (essentially a compiled program that can operate on the database).

### Front-End Consists of:

- Tokenizer
- Parser
- Code Generator

### Back-End Consists of:

- Virtual Machine (takes the bytecode to get instruction like a switch statement)
- B-tree (stores the table)
- Pager (reading and writing the correct page)
- OS Interface

### Phase 1: REPL Interface

```c
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>

typedef struct {
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

InputBuffer *new_input_buffer() {
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}
```

### Notes:

- `size_t` is an unsigned integer data type used to represent the size of an object in memory or the result of the `sizeof` operator.
- `ssize_t` is a signed integer version of `size_t`.
- `*` symbol is used to indicate a pointer. A pointer is a variable that stores the memory address of another variable.

---

## 27/08/2024

```c
void print_prompt() {
    printf("db>");
}

void read_input(InputBuffer *inputBuffer) {
    ssize_t bytes_read = getline(&(inputBuffer->buffer), &(inputBuffer->buffer_length), stdin);
    if (bytes_read <= 0) {
        printf("error in reading\n");
        exit(EXIT_FAILURE);
    }
    inputBuffer->buffer[bytes_read - 1] = 0;
    inputBuffer->input_length = bytes_read - 1;
}

void close_input_buffer(InputBuffer *inputBuffer) {
    free(inputBuffer->buffer);
    free(inputBuffer);
}

int main(int argc, int *argv) {
    InputBuffer *inputBuffer = new_input_buffer();
    while (1) {
        print_prompt();
        read_input(inputBuffer);
        if (strcmp(inputBuffer->buffer, ".exit") == 0) {
            close_input_buffer(inputBuffer);
            exit(EXIT_SUCCESS);
        } else {
            printf("Unrecognizable command '%s'", inputBuffer->buffer);
        }
    }
}
```

### Explanation:

To imitate the database interface, the `print_prompt()` function is made. Inside it, you'll find the first lines of what you want as the prompt. There are two functions here: `read_input` and `close_input_buffer`.

#### `read_input()`:

This reads the input buffer, like the commands you enter. The `getline()` function is used to get the size of the buffer to `bytes_read` and puts the command in `input_buffer`.

##### `getline()` Function:

`getline` reads a line of input and stores it in a buffer. If `*lineptr` is `NULL`, it allocates a buffer and updates `*n` with its size. If `*lineptr` is not `NULL`, it uses the provided buffer, resizing if needed. It returns the number of characters read, including the newline but not the null terminator. If an error occurs or end-of-file is reached, it returns `-1`.

The buffer, which stores the command's last character, is changed to `0` from "null character," and the `input_length` is decreased by 1.

#### `close_input_buffer()`:

Deallocates memory from the input buffer.

### Notes:

- `buffer`: A place (a memory buffer) where the input line is stored.
- `buffer_length`: The size of the buffer.
- `input_length`: The number of characters actually read from the input.
- The `getline` function in C is used to read an entire line of text from a stream, typically from standard input.
- `bytes_read` holds the number of characters that `getline` has read from the input, including the newline character.

---

## 31/08/2024

### Phase 2: SQL Compiler and Virtual Machine

We need to convert whatever SQL code we get to bytecode. The process is as follows:

- SQL Command --> Tokenizer --> Parser --> Bytecode --> Virtual Machine

---

## 03/09/2024

When getting an SQL command, you need to check if it is a meta command like `.exit`. So, I used `enum` to represent the meta command and a `do_meta_command` function to execute them.

```c
typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

MetaCommandResult do_meta_command(InputBuffer *input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
```

The other output we get is the actual SQL statement. We need to divide the SQL statement into what category it belongs to. The categories include DDL, DML, and so on.

We take the statement and give a type to it.

```c
typedef enum {
    SELECT_STATEMENT,
    INSERT_STATEMENT
} StatementType;

typedef struct {
    StatementType type;
} Statement;

PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement) {
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        statement->type = INSERT_STATEMENT;
        return PREPARE_SUCCESS;
    } else if (strncmp(inputBuffer->buffer, "select", 6) == 0) {
        statement->type = SELECT_STATEMENT;
        return PREPARE_SUCCESS;
    } else {
        return PREPARE_UNRECOGNIZED_COMMAND;
    }
}
```

Then we execute the statement.

---

## 09/09/2024

### Phase 3: A Single Table Database

To be precise, an in-memory, append-only, single-table database.

### Updated `Statement` Struct:

We add a row to insert.

```c
typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;
```

### Row Structure:

```c
#define COL_USERNAME_SIZE 32
#define COL_EMAIL_SIZE 255

// Struct that represents the syntax of a row
typedef struct {
    uint32_t id;
    char userName[COL_USERNAME_SIZE];
    char email[COL_EMAIL_SIZE];
} Row;
```

With a new `Statement` and `Row` struct, we update `prepare_statement()`:

```c
PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement) {
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0) {
        statement->type = INSERT_STATEMENT;
        int args_assigned = sscanf(inputBuffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.userName, statement->row_to_insert.email);
        if (args_assigned < 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    } else if (strncmp(inputBuffer->buffer, "select", 6) == 0) {
        statement->type = SELECT_STATEMENT;
        return PREPARE_SUCCESS;
    } else {
        return PREPARE_UNRECOGNIZED_COMMAND;
    }
}
```

### Assigning Data to `row_to_insert` in `Statement`

After the statement type is assigned, the data in the row is assigned to `row_to_insert` in the `Statement`. We will convert the given data into a data structure to represent the table.

### Data Structure Representation

Typically, a B-tree is used for representing the table. However, initially, we'll use an array. The structure will be as follows:

```
Table -----> Page -----> Rows -----> Data
```

Each row is an array. To retrieve data like the username, you just need the offset that represents the position of the username in the row.

```c
// gives the size of attribute in the structure
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

// size of inputs
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, userName);

// Offset size
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
```

### Column Size and Offset

| Column    | Size (bytes) | Offset |
| --------- | ------------ | ------ |
| ID        | 4            | 0      |
| Username  | 32           | 4      |
| Email     | 255          | 36     |
| **Total** | **291**      |        |

### Data Serialization and Deserialization

We need to convert data structures or objects into a format that can be easily stored or transmitted and then reconstructed later.

**Serialization:** Store the data.

**Deserialization:** Retrieve the data.

```c
void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->userName), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), (source + ID_OFFSET), ID_SIZE);
    memcpy(&(destination->userName), (source + USERNAME_OFFSET), USERNAME_SIZE);
    memcpy(&(destination->email), (source + EMAIL_OFFSET), EMAIL_SIZE);
}
```

**Note:**

- `memcpy` is a standard library function in C used to copy a block of memory from one location to another.
- `memcpy` returns a pointer to the destination memory area.
- It is used to serialize complex data structures into a byte array for storage, converting the data structure into a linear sequence of bytes.

### Finding the Row Slot in Memory

To perform operations, we need to determine the exact memory location to perform the operations.

```c
void *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    // There is a fixed number of rows in a page. Dividing row_num by ROWS_PER_PAGE gives the page number.
    // Example: 121st row will be on the 2nd page (121/100 = 1.21, index will be 2nd page).

    void *page = table->pages[page_num];
    if (page == NULL)
    {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    // There is a fixed number of rows in a page. Taking the remainder of row_num divided by ROWS_PER_PAGE gives the row's index within the page.
    // Example: If ROWS_PER_PAGE is 100, the 121st row will be the 21st row within its page (121 % 100 = 21).

    uint32_t byte_offset = ROW_SIZE * row_offset;
    // Each row occupies a fixed number of bytes (ROW_SIZE). Multiplying ROW_SIZE by row_offset gives the byte offset within the page where the row's data starts.
    // Example: If ROW_SIZE is 291 bytes and row_offset is 21, the data for the 121st row starts at the 6111th byte within the page (291 * 21 = 6111).

    return page + byte_offset;
    // This will give you the position of data (row) in a page.
}
```

---
