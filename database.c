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

// size of inputs
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, userName);

// Offset size
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

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
// struct that represents the sytax of insert
typedef struct
{
    uint32_t id;
    char userName[COL_USERNAME_SIZE];
    char email[COL_EMAIL_SIZE];

} Row;
typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR
} PrepareResult;
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

void print_prompt()
{
    printf("db>");
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

PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement)
{
    if (strncmp(inputBuffer->buffer, "insert", 6) == 0)
    {
        statement->type = INSERT_STATEMENT;
        int args_assigned = sscanf(inputBuffer->buffer, "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.userName, statement->row_to_insert.email);
        if (args_assigned < 3)
        {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
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

void executeStatement(Statement *statement)
{
    switch (statement->type)
    {
    case SELECT_STATEMENT:
        /* code */
        break;
    case INSERT_STATEMENT:
        break;
    default:
        break;
    }
}

int main(int argc, int *argv)
{
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
        case PREPARE_UNRECOGNIZED_COMMAND:
            printf("Unrecognized command at the start %s \n", inputBuffer->buffer);
        }
        executeStatement(&statement);
        printf("Executed.\n");
    }
}
