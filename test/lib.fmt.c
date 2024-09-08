#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "fmt.h"

UTEST(FMT, IntegerTypes)
{
    Arena a = arena_new(1024, vd_memory_get_system_allocator());

#define TESTCONV(typ, num, numlit)                     \
    {                                                  \
        arena_reset(&a);                               \
        str s = vd_snfmt(&a, "%{" #typ "}", numlit);   \
        EXPECT_TRUE(vd_str_eq(s, str_lit(#num)));      \
    }

    TESTCONV(u64, 18446744073709551615, 18446744073709551615ULL);
    TESTCONV(u32, 4294967295,           4294967295UL);
    TESTCONV(u16, 65535,                65535U);
    TESTCONV(u8,  255,                  255);
    TESTCONV(i64, -9223372036854775807, -9223372036854775807LL);
    TESTCONV(i32, -2147483647,          -2147483647L);
    TESTCONV(i16, -32767,               -32767);
    TESTCONV(i8,  -127,                 -127);

#undef TESTCONV

    arena_free(&a);
}

UTEST(FMT, StringSlices)
{
    Arena a = arena_new(1024, vd_memory_get_system_allocator());
    str s = vd_snfmt(&a, "%{stru32}", str_lit("Hello"));
    EXPECT_TRUE(vd_str_eq(s, str_lit("Hello")));
    arena_free(&a);
}

UTEST(FMT, MultipleStringSlices)
{
    Arena a = arena_new(1024, vd_memory_get_system_allocator());
    str s = vd_snfmt(&a, "%{stru32} %{stru32}", str_lit("Hello"), str_lit("World"));
    EXPECT_TRUE(vd_str_eq(s, str_lit("Hello World")));
    arena_free(&a);
}