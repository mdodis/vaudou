#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "strmap.h"
#include "intmap.h"
#include "handlemap.h"

UTEST(strmap, basic)
{
    strmap int *map = 0;
    strmap_init(map, vd_memory_get_system_allocator());

    int value = 1;
    strmap_set(map, str_lit("hello"), &value);

    int result;
    ASSERT_TRUE(strmap_get(map, str_lit("hello"), &result));
    ASSERT_EQ(result, 1);
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

UTEST(intmap, basic)
{
    VD_IntMap map;
    vd_intmap_init(&map, vd_memory_get_system_allocator(), 10, VD_INTMAP_FLAG_NO_AUTOGROW);

    ASSERT_TRUE(vd_intmap_set(&map, 1, 1));
    ASSERT_TRUE(vd_intmap_set(&map, 2, 2));
    ASSERT_TRUE(vd_intmap_set(&map, 3, 3));
    ASSERT_TRUE(vd_intmap_set(&map, 4, 4));
    ASSERT_TRUE(vd_intmap_set(&map, 5, 5));

    for (int i = 0; i < 5; ++i) {
        u64 v;

        ASSERT_TRUE(vd_intmap_tryget(&map, i + 1, &v));

        ASSERT_EQ(v, i + 1);
    }

    vd_intmap_deinit(&map);
}

UTEST(intmap, grow)
{
    VD_IntMap map;
    vd_intmap_init(&map, vd_memory_get_system_allocator(), 10, 0);

    for (int i = 1; i <= 11; ++i) {
        ASSERT_TRUE(vd_intmap_set(&map, i, i));
    }

    for (int i = 1; i <= 11; ++i) {
        u64 v;

        ASSERT_TRUE(vd_intmap_tryget(&map, i, &v));

        ASSERT_EQ(v, i);
    }

    vd_intmap_deinit(&map);
}

static void string_free(void *object, void *c) {
    free(object);
}

UTEST(handlemap, basic)
{
    VD_HANDLEMAP const char **map;
    VD_HANDLEMAP_INIT(map, {
        .allocator = vd_memory_get_system_allocator(),
        .on_free_object = string_free,
        .initial_capacity = 10,
    });

    const char *new_value = _strdup("value");
    VD_Handle handle = VD_HANDLEMAP_REGISTER(map, &new_value, {
        .ref_mode = VD_HANDLEMAP_REF_MODE_COUNT
    });

    ASSERT_NE(handle.id, 0);
    ASSERT_NE(handle.map, (void*)0);

    const char *use_value = *(const char**)vd_handle_use(&handle, 0);

    ASSERT_STREQ(use_value, "value");
}
