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

## 23/09/2024

### functions to initialize and free the table

For starters lets add functions to initialize and free the table.

```c
Table *new_table()
{
    Table *table = (Table *)malloc(sizeof(Table));
    table->num_rows = 0;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        table->pages[i] = NULL;
    }
    return table;
}
void free_table(Table *table)
{
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        free(table->pages[i]);
    }
    free(table);
}
```

### Print row

The following function will be used to print the row:

```c
void print_row(Row *row)
{
    printf("(%d,%s,%s)\n", row->id, row->userName, row->email);
}
```

Make a enum to define execution result

```c
typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL

} ExecuteResult;
```

Now we need functions to execute the insert and select command.

### Insertion

For the insertion command

```c
ExecuteResult execute_insert(Statement *statement, Table *table)
{
    if (table->num_rows == TABLE_MAX_PAGES)
    {
        return EXECUTE_TABLE_FULL;
    }
    Row *row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;
    return EXECUTE_SUCCESS;
}
```

We check if the number of rows ina table has been been maxed out.
Then we insert the data to the table through `serialize_row` function.

### Selection

```c
ExecuteResult execute_select(Statement *statement, Table *table)
{
    Row row;
    for (uint32_t i = 0; i < table->num_rows; i++)
    {
        deserialize_row(row_slot(table, i), &row);
    }
    print_row(&(row));
    return EXECUTE_SUCCESS;
}
```

We create a temporary Row (memory for row info) the add all the data in the table to the row.
We make changes in execute_statement and main funtions.

**Notes**:

- Having \* in front of a function return type indicates that the function returns a pointer to a particular data type.
- If a \* is there in the functions parameters it indicates that you need to pass address of the variable not the value of the variable.
- If you want to pass the address of the variable you can use &. Ex: &(your_variable);

## 29/09/2024

### add additional 1 byte for "\0" in the Row email and username

```c
typedef struct
{
    uint32_t id;
    char userName[COL_USERNAME_SIZE + 1];
    char email[COL_EMAIL_SIZE + 1];
    // additional 1 byte for "\0" in the string

} Row;
```

### Better Parsing for insert

In scanf if the string reading is larger than the buffer then it allocates the data in random places.
So We want to check the length of each string before we copy it into a Row structure.
So we use strtok() and strlen()
First we will tokenize the statement and store it in variables.
These variable include userName, id, email.
The variables will be checked for NEGATIVE values and their length.

```c
typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_IS_TOO_LONG
} PrepareResult;
PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement)
{
    statement->type = INSERT_STATEMENT;
    const *keyword = strtok(input_buffer->buffer, " ");
    const *id_STRING = strtok(NULL, " ");
    const *userName = strtok(NULL, " ");
    const *email = strtok(NULL, " ");
    if (id_STRING == NULL || userName == NULL || email == NULL)
    {
        return PREPARE_SYNTAX_ERROR;
    }
    int id = atoi(id_STRING);
    if (id < 0)
    {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(userName) < COL_USERNAME_SIZE)
    {
        return PREPARE_STRING_IS_TOO_LONG;
    }
    if (strlen(email) < COL_EMAIL_SIZE)
    {
        return PREPARE_STRING_IS_TOO_LONG;
    }
    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.userName, userName);
    strcpy(statement->row_to_insert.email, email);
    return PREPARE_SUCCESS;
}
```

**Notes**:

- `strtok()` is used in this case to split a string into smaller parts (tokens) based on a specified delimiter, which is a space (" ") in this project.
- It replaces the space character with a null terminator ('\0').
- Subsequent calls to `strtok()` with NULL as the first argument continue tokenizing the original string by finding the next delimiter and replacing it with a null terminator.
- The way `strtok()` knows where it left off is by using an internal static pointer to track its position in the original string.
- `strcpy()` does not copy the address of a string; it copies the contents

## 11/10/2024

### Phase 4: Persistent Database

### We need to make the data persist. For now we are using a file to store the data.

We will use pager abstraction to make it easier.
Pager will store data if there is a cache miss the file in the disk will be searched.
Pager acts as a page cache.

```c
typedef struct
{
    int file_descriptor;
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;
```

Add pager to the Table struct.

### Function `pager_open()` will open the file with necessary permissions.

```c
Pager *pager_open(const char *filename)
{

    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1)
    {
        printf("Unable to open the file.\n");
        exit(EXIT_FAILURE);
    }
    off_t file_length = lseek(fd, 0, SEEK_END);
    Pager *pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pager->pages[i] = NULL;
    }
    return pager;
}
```

It opens the file and stores the size of the file.

**Notes:**

- `open()` is a system call returns a non-negative integer if opened, which is the file descriptor (fd). If it fails, it returns -1.
- A file descriptor is an integer that uniquely identifies an open file within a process
- The `lseek()` function in C is used to move the file offset (the current position in the file) to a different location.
- `lseek()` allows random access in files, meaning you can jump to any point in the file without reading through it sequentially.
- `off_t lseek(int fd, off_t offset, int whence)`.
- whence determines how the offset is applied.

### Change `new_table()` to `db_open()` with initializing table and page structures and opening database.

```c
Table *db_open(const char *fileName)
{

    Pager *pager = pager_open(fileName);
    uint32_t num_rows = pager->file_length / ROW_SIZE;
    Table *table = malloc(sizeof(table));
    table->pager = pager;
    table->num_rows = num_rows;
    return table;
}
```

### We have a function `get_page()` that is logic for handling a cache miss.

```c
void *get_page(Pager *pager, uint32_t page_num)
{
    if (page_num > TABLE_MAX_PAGES)
    {
        printf("Tried to fetch page number out of bounds. %d > %d\n", page_num,
               TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL)
    {
        // cache miss
        void *page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length / PAGE_SIZE;
        // page only has partial data
        if (pager->file_length % PAGE_SIZE)
        {
            num_pages += 1;
        }
        if (page_num <= num_pages)

        {
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
            if (bytes_read == -1)
            {
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }
        pager->pages[page_num] = page;
    }
    return pager->pages[page_num];
}
```

checks if the page number that is greate than max, then checks if the page is NULL if it is null then allocates the memory and stores the data from the memory.
Also checks for partial page save.

### When the user exits, we’ll call a new method called db_close()

- flushes the page cache to disk
- closes the database file
- frees the memory for the Pager and Table data structures

```c
void *db_close(Table *table)
{
    Pager *pager = table->pager;
    uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;
    for (uint32_t i = 0; i < num_full_pages; i++)
    {
        if (pager->pages[i] == NULL)
        {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // if there is additional data
    uint32_t num_additional_page = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_page >= 1)
    {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != NULL)
        {
            pager_flush(pager, page_num, num_additional_page * ROW_SIZE);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }
    int result = close(pager->file_descriptor);
    if (result == -1)
    {
        printf("error in clsoing.\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        void *page = pager->pages[i];
        if (page)
        {
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}
```

### `pager_flush()`, is responsible for writing a page from memory (the cache) back to the file on disk.

The design currently keeps track of how many rows are in the database by the length of the file. Because of
that, when writing the final page (which may be only partially filled), we have to write a "partial page"
(i.e., not the full PAGE_SIZE) to the end of the file. This is why pager_flush() takes both a page_num and a
size—the size specifies how many bytes of the page need to be written.

```c
void *pager_flush(Pager *pager, uint32_t page_num, uint32_t size)
{
    if (pager->pages[page_num] == NULL)
    {
        printf("tried to flush empty page");
        exit(EXIT_FAILURE);
    }
    off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
    if (offset == -1)
    {
        printf("Error seeking.\n");
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);
    if (bytes_written == -1)
    {
        printf("error in writing.\n");
        exit(EXIT_FAILURE);
    }
}
```

The data from the pager is flushed to the memory by `write()` function.

Also change the main function:

```c
int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Need to enter the File name.\n");
        exit(EXIT_FAILURE);
    }
    char *filename = argv[1];
    Table *table = db_open(filename);
    InputBuffer *inputBuffer = new_input_buffer();
    while (true)
    {
        print_prompt();
        read_input(inputBuffer);

        if ((inputBuffer->buffer[0] == '.'))
        {
            switch (do_meta_command(inputBuffer, table))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized command '%s'\n", inputBuffer->buffer);
                continue;
            }
        }
        Statement statement;
        switch (prepare_statement(inputBuffer, &statement))
        {
        case PREPARE_SUCCESS:

            break;

        case PREPARE_NEGATIVE_ID:
            printf("ID can't be negative.\n");
            continue;
        case PREPARE_UNRECOGNIZED_COMMAND:
            printf("Unrecognized command at the start %s \n", inputBuffer->buffer);
            continue;
        case PREPARE_STRING_IS_TOO_LONG:
            printf("string is too long.\n");
            continue;
        case PREPARE_SYNTAX_ERROR:
            printf("Syntax error. Could not parse statement.\n");
            continue;
        }
        switch (execute_statement(&statement, table))
        {
        case EXECUTE_SUCCESS:
            printf("Executed.\n");
            break;
        case EXECUTE_TABLE_FULL:
            printf("Table Full\n");
        }
    }
}
```

When running the file with the ./a.out command add the file name.
For example : `./a.out db.db`

## 18/10/2024

### Phase 5: Cursor Abstraction

### We add cursor for:

- Create a cursor at the beginning of the table
- Create a cursor at the end of the table
- Access the row the cursor is pointing to
- Advance the cursor to the next row

### Cursor Structure:

```c
typedef struct
{
    Table *table;
    uint32_t row_num;
    bool end_of_table;// represents if the the cursor is at the end
} Cursor;
```

### To move cursor to the starting of the table:

```c
Cursor *table_start(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}
```

### To move cursor to the end of the table:

```c
Cursor *table_end(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;
    return cursor;
}

```

### change the `row_slot()` function so that it can return the row which the cursor is pointing to:

```c
void *cursor_value(Cursor *cursor)
{
    uint32_t row_num = cursor->row_num;
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void *page = get_page(cursor->table->pager, page_num);
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = ROW_SIZE * row_offset;
    return page + byte_offset;
    // will give you the position of data(row) in a page
}
```

### To go to the next row with the cursor

```c
void cursor_advance(Cursor *cursor)
{
    cursor->row_num += 1;

    if (cursor->row_num >= cursor->table->num_rows)
    {
        cursor->end_of_table = true;
    }
}
```

### Change the `execut_select()` and `execute_insert()` functions to manage the cursor

```c
ExecuteResult execute_insert(Statement *statement, Table *table)
{

    if (table->num_rows >= TABLE_MAX_ROWS)
    {
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);
    print_row(row_to_insert);
    Cursor *cursor = table_end(table);
    serialize_row(row_to_insert, cursor_value(cursor));
    table->num_rows += 1;
    free(cursor);
    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement *statement, Table *table)
{
    Cursor *cursor = table_start(table);
    Row row;

    while (!(cursor->end_of_table))
    {
        deserialize_row(cursor_value(cursor), &(row));
        print_row(&(row));
        cursor_advance(cursor);
    }

    return EXECUTE_SUCCESS;
}
```
