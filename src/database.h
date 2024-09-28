#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include <stdint.h>

// Constants and Macros
#define COL_USERNAME_SIZE 32
#define COL_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

// Structs and Enums

// Represents a row in the database
typedef struct
{
    uint32_t id;
    char userName[COL_USERNAME_SIZE];
    char email[COL_EMAIL_SIZE];
} Row;

// Represents the database table
typedef struct
{
    uint32_t num_rows;
    void *pages[TABLE_MAX_PAGES];
} Table;

// Buffer for user input
#include <sys/types.h>

typedef struct
{
    char *buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

// Result types for meta commands
typedef enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

// Result types for statement preparation
typedef enum
{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_COMMAND,
    PREPARE_SYNTAX_ERROR
} PrepareResult;

// Result types for statement execution
typedef enum
{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

// Types of statements
typedef enum
{
    SELECT_STATEMENT,
    INSERT_STATEMENT
} StatementType;

// Represents a SQL statement
typedef struct
{
    StatementType type;
    Row row_to_insert; // Used by INSERT_STATEMENT
} Statement;

// Function Declarations

// Input buffer functions
InputBuffer *new_input_buffer();
void read_input(InputBuffer *inputBuffer);
void close_input_buffer(InputBuffer *inputBuffer);

// Table functions
Table *new_table();
void free_table(Table *table);

// Row functions
void print_row(Row *row);
void *row_slot(Table *table, uint32_t row_num);
void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);

// Meta command handler
MetaCommandResult do_meta_command(InputBuffer *input_buffer);

// Statement preparation and execution
PrepareResult prepare_statement(InputBuffer *inputBuffer, Statement *statement);
ExecuteResult execute_statement(Statement *statement, Table *table);
ExecuteResult execute_insert(Statement *statement, Table *table);
ExecuteResult execute_select(Statement *statement, Table *table);

// Utility functions
void print_prompt();

#endif /* DATABASE_H */
