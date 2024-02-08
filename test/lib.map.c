#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "strmap.h"

UTEST(strmap, basic)
{
    strmap int *map = 0;
    strmap_init(map, vd_memory_get_system_allocator());

    int value = 0;
    strmap_set(map, str_lit("hello"), &value);

    int result;
    ASSERT_TRUE(strmap_get(map, str_lit("hello"), &result));
    ASSERT_EQ(result, 0);
}

UTEST(strmap, update)
{
    int result;
    strmap int *map = 0;
    strmap_init(map, vd_memory_get_system_allocator());

    int value = 0;
    strmap_set(map, str_lit("hello"), &value);

    ASSERT_TRUE(strmap_get(map, str_lit("hello"), &result));
    ASSERT_EQ(result, 0);

    value = 1;
    strmap_set(map, str_lit("hello"), &value);

    ASSERT_TRUE(strmap_get(map, str_lit("hello"), &result));
    ASSERT_EQ(result, 1);
}

UTEST(strmap, delete)
{
    int result;
    strmap int *map = 0;
    strmap_init(map, vd_memory_get_system_allocator());

    int value = 0;
    strmap_set(map, str_lit("hello"), &value);

    ASSERT_TRUE(strmap_get(map, str_lit("hello"), &result));
    ASSERT_EQ(result, 0);

    strmap_del(map, str_lit("hello"));

    ASSERT_FALSE(strmap_get(map, str_lit("hello"), &result));
}