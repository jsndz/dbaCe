#include <criterion/criterion.h>
Test(sample, test1)
{
    cr_assert_eq(2 + 2, 4, "Expected 2 + 2 to equal 4");
}
