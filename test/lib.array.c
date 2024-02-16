#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "array.h"

UTEST(array, test_add_basic)
{
    dynarray int *arr = 0;
    array_init(arr, vd_memory_get_system_allocator());

    array_add(arr, 1);
    array_add(arr, 2);
    array_add(arr, 3);

    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

UTEST(array, test_add_grow)
{
    int *arr = 0;
    array_init(arr, vd_memory_get_system_allocator());

    for (int i = 0; i < 100; i++)
    {
        array_add(arr, i);
    }

    for (int i = 0; i < 100; i++)
    {
        EXPECT_EQ(arr[i], i);
    }
}