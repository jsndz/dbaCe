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
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY

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
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;

// Internal Node Header Format
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           +INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;
uint32_t *leaf_node_num_cells(void *node)
{
    // returns a pointer to the number of cells(key - value pairs) stored in a leaf node of a B - tree.
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}
void *leaf_node_cell(void *node, uint32_t cell_num)
{
    // location of the leaf node
    return node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}
uint32_t *leaf_node_key(void *node, uint32_t cell_num)
{
    // location of the leaf node key
    return leaf_node_cell(node, cell_num);
}
void *leaf_node_value(void *node, uint32_t cell_num)
{
    // location of the leaf node value
    return leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}
void set_node_type(void *node, NodeType type)
{
    uint8_t value = type;
    *((uint8_t *)(node + NODE_TYPE_OFFSSET)) = value;
}
void set_root_node(void *node, bool is_root)
{
    uint8_t value = is_root;
    *((uint32_t *)(node + IS_ROOT_OFFSET)) = value;
}
void *initialize_leaf_node(void *node)
{
    // create a  leaf node
    memset(node, 0, PAGE_SIZE);
    set_node_type(node, NODE_LEAF);
    *leaf_node_num_cells(node) = 0;
    set_root_node(node, false);
}

// internal node
uint32_t *internal_node_num_key(void *node)
{
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}
void *initialize_internal_node(void *node)
{
    memset(node, 0, PAGE_SIZE);
    set_node_type(node, NODE_INTERNAL);
    set_root_node(node, false);
    *internal_node_num_key(node) = 0;
}

uint32_t *internal_node_right_child(void *node)
{
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}
uint32_t *internal_node_cell(void *node, uint32_t cell_num)
{
    return node + INTERNAL_NODE_HEADER_SIZE + cell_num * INTERNAL_NODE_CELL_SIZE;
}
uint32_t *internal_node_child(void *node, uint32_t child_num)
{
    uint32_t num_keys = *internal_node_num_key(node);
    if (child_num > num_keys)
    {
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    }
    else if (child_num == num_keys)
    {
        return internal_node_right_child(node);
    }
    else
    {
        return internal_node_cell(node, child_num);
    }
}
uint32_t *internal_node_key(void *node, uint32_t key_num)
{
    return internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE;
}
NodeType get_node_type(void *node)
{
    uint32_t value = *((uint8_t *)(node + NODE_TYPE_OFFSSET));

    return (NodeType)value;
}

uint32_t get_node_max_key(void *node)
{
    switch (get_node_type(node))
    {
    case NODE_INTERNAL:
        return *internal_node_key(node, *internal_node_num_key(node) - 1);

    case NODE_LEAF:
        return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
    }
}
bool is_root_node(void *node)
{
    uint8_t value = *((uint32_t *)(node + IS_ROOT_OFFSET));
    return value != 0;
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
Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key)
{
    void *node = get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;
    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index)
    {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(node, index);
        if (key == key_at_index)
        {
            cursor->cell_num = index;
            return cursor;
        }
        if (key < key_at_index)
        {
            one_past_max_index = index;
        }
        else
        {
            min_index = index + 1;
        }
    }
    cursor->cell_num = min_index;
    return cursor;
}

void print_constants()
{
    printf("Constants:\n");
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

Cursor *
table_start(Table *table)
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
Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key)
{
    void *node = get_page(table->pager, page_num);
    uint32_t num_keys = *internal_node_num_key(node);
    uint32_t min_index = 0;
    uint32_t max_index = num_keys;
    while (max_index != min_index)
    {
        uint32_t index = (min_index + max_index) / 2;
        uint32_t key_to_right = *internal_node_key(node, index);

        if (key < key_to_right)
        {
            max_index = index;
        }
        else
        {
            min_index = index + 1;
        }
    }
}

Cursor *table_find(Table *table, uint32_t key)
{
    uint32_t root_page_num = table->root_page_num;
    void *root_node = get_page(table->pager, root_page_num);
    if (get_node_type(root_node) == NODE_LEAF)
    {
        return leaf_node_find(table, root_page_num, key);
    }
    else
    {
        return internal_node_find(table, root_page_num, key);
    }
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
    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;
    if (pager->num_pages == 0)
    {
        // New DB file.
        void *root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
        set_root_node(root_node, true);
    }
    return table;
}

uint32_t get_unused_pages(Pager *pager)
{
    return pager->num_pages;
}

void *create_new_root_node(Table *table, uint32_t right_child_page_num)
{
    void *root = get_page(table->pager, table->root_page_num);
    void *right_child = get_page(table->pager, right_child_page_num);
    uint32_t left_child_page_num = get_unused_pages(table->pager);
    void *left_child = get_page(table->pager, left_child_page_num);
    memcpy(left_child, root, PAGE_SIZE);
    set_root_node(left_child, false);
    initialize_internal_node(root);
    set_root_node(root, true);
    *internal_node_num_key(root) = 1;
    *internal_node_child(root, 0) = left_child_page_num;
    uint32_t left_child_max_key = get_node_max_key(left_child);
    *internal_node_key(root, 0) = left_child_max_key;
    *internal_node_right_child(root) = right_child_page_num;
}
void *leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value)
{
    // get old node
    void *old_node = get_page(cursor->table->pager, cursor->page_num);
    // at the end of the page create new node
    uint32_t new_page_num = get_unused_pages(cursor->table->pager);
    void *new_node = get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);

    for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--)
    {
        void *destination_node;

        // get the destination where to go
        // if the node is bigger than the middle one then should go to right node
        if (i >= LEAF_NODE_LEFT_SPLIT_COUNT)
        {
            destination_node = new_node;
        }
        else
        {
            destination_node = old_node;
        }
        // to get the index within node
        // if it is the biggest id node it should go to right most on new node
        uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        // eg: if the max is 4 and 5 elements are there the biggest will be in 5 % 2 ==1 so rightmost
        void *destination = leaf_node_cell(destination_node, index_within_node);
        if (i == cursor->cell_num)
        {
            serialize_row(value, destination);
        }
        else if (i > cursor->cell_num)
        {
            memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
        }
        else
        {
            memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
        }

        /*
        Loop each cell
        Split the cells based on the its old node poisition
        find index within the old or new node
        find the destination node cell
        if the node is the same on as the cursor is pointing to just serialize row
        if not copy them to old node or new node
        */
        *(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
        *(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
        if (is_root_node(old_node))
        {
            return create_new_root_node(cursor->table, new_page_num);
        }
        else
        {
            printf("Need to implement update root");
            exit(EXIT_FAILURE);
        }
    }
}
void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value)
{

    void *node = get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS)
    {
        leaf_node_split_and_insert(cursor, key, value);
        return;
    }
    if (cursor->cell_num < num_cells)
    {
        for (uint32_t i = num_cells; i > cursor->cell_num; i--)
        {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
        }
    }
    *(leaf_node_key(node, cursor->cell_num)) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
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
void indent(uint32_t level)
{
    for (uint32_t i = 0; i < level; i++)
    {
        printf("  ");
    }
}
void print_tree(Pager *pager, uint32_t page_num, uint32_t indentation_level)
{
    void *node = get_page(pager, page_num);
    uint32_t num_keys, child;
    switch (get_node_type(node))
    {
    case NODE_LEAF:
        num_keys = *leaf_node_num_cells(node);
        printf("%d hh", num_keys);
        indent(indentation_level);
        printf("-leaf (size %d)\n", num_keys);
        for (uint32_t i = 0; i < num_keys; i++)
        {
            indent(indentation_level + 1);
            printf("- %d", *leaf_node_key(node, i));
        }
        break;
    case NODE_INTERNAL:
        num_keys = *internal_node_num_key(node);
        indent(indentation_level);
        printf("-internal (size %d)\n", num_keys);
        for (uint32_t i = 0; i < num_keys; i++)
        {
            // gets the child node
            child = *internal_node_child(node, i);
            // recursively calling for the child node
            print_tree(pager, child, indentation_level + 1);
            indent(indentation_level + 1);
            printf("-key %d\n", *internal_node_key(node, i));
        }
        break;
    }
}
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table)
{
    if (strcmp(input_buffer->buffer, ".exit") == 0)
    {
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(input_buffer->buffer, ".constants") == 0)
    {
        print_constants();
        return META_COMMAND_SUCCESS;
    }
    else if (strcmp(input_buffer->buffer, ".btree") == 0)
    {
        printf("Tree:\n");
        print_tree(table->pager, 0, 0);
        return META_COMMAND_SUCCESS;
    }
    else
    {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}
void *cursor_value(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    void *page = get_page(cursor->table->pager, page_num);
    return leaf_node_value(page, cursor->cell_num);
    // will give you the position of data(row) in a page
}
void cursor_advance(Cursor *cursor)
{
    uint32_t page_num = cursor->page_num;
    void *node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;

    if (cursor->cell_num >= (*leaf_node_num_cells(node)))
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
    void *node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Row *row_to_insert = &(statement->row_to_insert);
    uint32_t key_to_insert = row_to_insert->id;
    Cursor *cursor = table_find(table, key_to_insert);
    if (cursor->cell_num < num_cells)
    {
        uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
        if (key_at_index == key_to_insert)
        {
            return EXECUTE_DUPLICATE_KEY;
        }
    }
    leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
    free(cursor);
    return EXECUTE_SUCCESS;
}
ExecuteResult execute_select(Statement *statement, Table *table)
{
    Cursor *cursor = table_start(table);
    Row row;
    printf("N");
    while (!(cursor->end_of_table))
    {

        printf("1");
        deserialize_row(cursor_value(cursor), &(row));
        printf("2");
        print_row(&(row));
        printf("3");
        cursor_advance(cursor);
        printf("4");
    }
    printf("M");
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
            break;
        case EXECUTE_DUPLICATE_KEY:
            printf("Key Already Exists.\n");
            break;
        }
    }
}
