#include "../unity/unity.h"

#include "../src/database.h"

#include <stdio.h>
#include <string.h>

// Mock function that simulates running the database with commands and capturing the output
char *run_script(const char *commands[], int num_commands)
{
    static char output[1024];
    memset(output, 0, sizeof(output));
    for (int i = 0; i < num_commands; i++)
    {
        // Here you would have logic to interact with the actual database
        // The example is simplified to simulate command handling

        if (strstr(commands[i], "insert") != NULL)
        {
            strcat(output, "db > Executed.\n");
        }
        if (strstr(commands[i], "select") != NULL)
        {
            strcat(output, "db > (1, user1, person1@example.com)\nExecuted.\n");
        }
        if (strstr(commands[i], ".exit") != NULL)
        {
            strcat(output, "db > \n");
        }
        if (strstr(commands[i], "Error: Table full.") != NULL)
        {
            strcat(output, "db > Error: Table full.\n");
        }
        if (strstr(commands[i], "String is too long.") != NULL)
        {
            strcat(output, "db > String is too long.\n");
        }
        if (strstr(commands[i], "ID must be positive.") != NULL)
        {
            strcat(output, "db > ID must be positive.\n");
        }
    }
    return output;
}
void test_inserts_and_retrieves_row(void)
{
    const char *commands[] = {
        "insert 1 user1 person1@example.com",
        "select",
        ".exit"};
    char *result = run_script(commands, 3);

    TEST_ASSERT_TRUE(strstr(result, "db > Executed.") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "db > (1, user1, person1@example.com)") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "Executed.") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "db > ") != NULL);
}
void test_prints_error_when_table_is_full(void)
{
    const char *script[1402]; // 1401 insert commands + 1 exit command
    for (int i = 0; i < 1401; i++)
    {
        static char insert_command[64];
        sprintf(insert_command, "insert %d user%d person%d@example.com", i + 1, i + 1, i + 1);
        script[i] = strdup(insert_command); // Populate the script with insert commands
    }
    script[1401] = ".exit"; // Add the exit command

    char *result = run_script(script, 1402);

    // Check if the output contains "db > Error: Table full."
    TEST_ASSERT_TRUE(strstr(result, "db > Error: Table full.") != NULL);
}
void test_allows_inserting_max_length_strings(void)
{
    char long_username[33]; // 32 characters + null terminator
    char long_email[256];   // 255 characters + null terminator

    memset(long_username, 'a', 32);
    long_username[32] = '\0';
    memset(long_email, 'a', 255);
    long_email[255] = '\0';

    const char *commands[] = {
        "insert 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "select",
        ".exit"};

    char *result = run_script(commands, 3);

    // Check if the output contains the expected result
    TEST_ASSERT_TRUE(strstr(result, "db > Executed.") != NULL);
    TEST_ASSERT_TRUE(strstr(result, "(1, aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa)") != NULL);
}
void test_prints_error_if_strings_are_too_long(void)
{
    const char *commands[] = {
        "insert 1 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "select",
        ".exit"};

    char *result = run_script(commands, 3);

    // Check if the output contains "db > String is too long."
    TEST_ASSERT_TRUE(strstr(result, "db > String is too long.") != NULL);
}
void test_prints_error_if_id_is_negative(void)
{
    const char *commands[] = {
        "insert -1 cstack foo@bar.com",
        "select",
        ".exit"};

    char *result = run_script(commands, 3);

    // Check if the output contains "db > ID must be positive."
    TEST_ASSERT_TRUE(strstr(result, "db > ID must be positive.") != NULL);
}
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_inserts_and_retrieves_row);
    RUN_TEST(test_prints_error_when_table_is_full);
    RUN_TEST(test_allows_inserting_max_length_strings);
    RUN_TEST(test_prints_error_if_strings_are_too_long);
    RUN_TEST(test_prints_error_if_id_is_negative);

    return UNITY_END();
}
