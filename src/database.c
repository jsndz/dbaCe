#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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

//

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct
{
    uint32_t num_rows;
    void *pages[TABLE_MAX_PAGES];
} Table;

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

InputBuffer *
new_input_buffer()
{
    InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}
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

MetaCommandResult do_meta_command(InputBuffer *input_buffer)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        exit(EXIT_SUCCESS);
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
void *row_slot(Table *table, uint32_t row_num)
{
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void *page = table->pages[page_num];
    if (page == NULL)
    {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = ROW_SIZE * row_offset;
    return page + byte_offset;
    // will give you the position of data(row) in a page
}
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
PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement)
{
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0)
    {
        prepare_insert(inputBuffer, statement);
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
    if (table->num_rows == TABLE_MAX_PAGES)
    {
        return EXECUTE_TABLE_FULL;
    }
    Row *row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;
    return EXECUTE_SUCCESS;
}
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

int main(int argc, int *argv)
{
    Table *table = new_table();
    InputBuffer *inputBuffer = new_input_buffer();
    while (1)
    {
        print_prompt();
        read_input(inputBuffer);
        if ((inputBuffer->buffer[0] == '.'))
        {
            switch (do_meta_command(inputBuffer))
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
        default:
            break;
        }
    }
}
