#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define COL_USERNAME_SIZE 32
#define COL_EMAIL_SIZE 255
// gives the size of attribute in the structure
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)
// compact representation of a row
//  struct that represents the syntax of row

typedef struct
{
    uint32_t id;
    char userName[COL_USERNAME_SIZE + 1];
    char email[COL_EMAIL_SIZE + 1];
    // additional 1 byte for "\0" in the string

} Row;
// size of inputs
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, userName);

// Offset size
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
typedef struct
{
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;
typedef struct
{
    uint32_t root_page_num;

    Pager *pager;
} Table;
typedef struct
{
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;
// structure for buffer input
typedef struct
{
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;
// structure for meta command result like ".exit"
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR,
    PREPARE_NEGATIVE_ID,
    PREPARE_USRNAME_STRING_IS_TOO_LONG,
    PREPARE_STRING_IS_TOO_LONG
} PrepareResult;
typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL

} ExecuteResult;
typedef enum
{
    SELECT_STATEMENT,
    INSERT_STATEMENT
} StatementType;

typedef struct
{
    StatementType type;
    Row row_to_insert;
} Statement;

typedef enum
{
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

// common NODE header Constants
const uint32_t NODE_TYPE_SIZE = sizeof(uint32_t);
const uint32_t NODE_TYPE_OFFSSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint32_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_SIZE + IS_ROOT_OFFSET;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// Leaf Node header constants
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// Leaf node body constants
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_VALUE_OFFSSET = 0;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

uint32_t *leaf_node_num_cells(void *node)
{
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}
void *leaf_node_cell(void *node, uint32_t cell_num)
{
    return node + LEAF_NODE_HEADER_SIZE + cell_num + LEAF_NODE_CELL_SIZE;
}
uint32_t *leaf_node_key(void *node, uint32_t cell_num)
{
    return leaf_node_cell(node, cell_num);
}
void *leaf_node_value(void *node, uint32_t cell_num)
{
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}
void *initialize_leaf_node(void *node)
{
    *leaf_node_num_cells(node) = 0;
}

Cursor *table_start(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->cell_num = 0;
    cursor->page_num = table->root_page_num;
    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);
    return cursor;
}
Cursor *table_end(Table *table)
{
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    void *root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);

    cursor->cell_num = num_cells;
    cursor->end_of_table = true;
    return cursor;
}

InputBuffer *
new_input_buffer()
{
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}
void serialize_row(Row *source, void *destination)
{
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    strncpy(destination + USERNAME_OFFSET, source->userName, USERNAME_SIZE);
    strncpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
}
void deserialize_row(void *source, Row *destination)
{
    memcpy(&(destination->id), (source + ID_OFFSET), ID_SIZE);
    memcpy(&(destination->userName), (source + USERNAME_OFFSET), USERNAME_SIZE);
    memcpy(&(destination->email), (source + EMAIL_OFFSET), EMAIL_SIZE);
}

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
    pager->num_pages = (file_length / PAGE_SIZE);
    if (file_length % PAGE_SIZE != 0)
    {
        printf("Corrupt file. \n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        pager->pages[i] = NULL;
    }
    return pager;
}
Table *db_open(const char *fileName)
{

    Pager *pager = pager_open(fileName);
    uint32_t num_rows = pager->file_length / ROW_SIZE;
    Table *table = malloc(sizeof(table));
    table->pager = pager;
    table->num_rows = num_rows;
    return table;
}

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

        // Logic of loading data from the disk
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
        if (page_num >= pager->num_pages)
        {
            pager->num_pages = page_num + 1;
        }
    }
    return pager->pages[page_num];
}
void *pager_flush(Pager *pager, uint32_t page_num)
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
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
    if (bytes_written == -1)
    {
        printf("error in writing.\n");
        exit(EXIT_FAILURE);
    }
}
void *db_close(Table *table)
{
    Pager *pager = table->pager;
    for (uint32_t i = 0; i < pager->num_pages; i++)
    {
        if (pager->pages[i] == NULL)
        {
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
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
void free_table(Table *table)
{
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++)
    {
        free(table->pager->pages[i]);
    }
    free(table);
}
void print_prompt()
{
    printf("db>");
}
void print_row(Row *row)
{
    printf("(%d,%s,%s)\n", row->id, row->userName, row->email);
}
void read_input(InputBuffer *inputBuffer)
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

void close_input_buffer(InputBuffer *inputBuffer)
{
    free(inputBuffer->buffer);
    free(inputBuffer);
}

MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
void *cursor_value(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    const *page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
    // will give you the position of data(row) in a page
}
void cursor_advance(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    void *node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;

    if (cursor->row_num >= (*leaf_node_num_cells(node)))
    {
        cursor->end_of_table = true;
    }
}
PrepareResult prepare_insert(InputBuffer *input_buffer, Statement *statement)
{
    statement->type = INSERT_STATEMENT;
    const char *keyword = strtok(input_buffer->buffer, " ");
    const char *id_STRING = strtok(NULL, " ");
    const char *userName = strtok(NULL, " ");
    const char *email = strtok(NULL, " ");
    if (id_STRING == NULL || userName == NULL || email == NULL)
    {
        return PREPARE_SYNTAX_ERROR;
    }
    int id = atoi(id_STRING);
    if (id < 0)
    {
        return PREPARE_NEGATIVE_ID;
    }
    if (strlen(userName) >= COL_USERNAME_SIZE)
    {
        return PREPARE_USRNAME_STRING_IS_TOO_LONG;
    }
    if (strlen(email) >= COL_EMAIL_SIZE)
    {
        return PREPARE_STRING_IS_TOO_LONG;
    }
    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.userName, userName);
    strcpy(statement->row_to_insert.email, email);
    return PREPARE_SUCCESS;
}
PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement)
{
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0)
    {

        return prepare_insert(inputBuffer, statement);
    }
    else if (strncmp(inputBuffer->buffer, "select", 6) == 0)
    {
        statement->type = SELECT_STATEMENT;
        return PREPARE_SUCCESS;
    }
    else
    {
        return PREPARE_UNRECOGNIZED_COMMAND;
    }
}
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
ExecuteResult execute_statement(Statement *statement, Table *table)
{
    switch (statement->type)
    {
    case SELECT_STATEMENT:
        execute_select(statement, table);
        break;
    case INSERT_STATEMENT:
        execute_insert(statement, table);
        break;
    default:
        break;
    }
}

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
