#define VD_ABBREVIATIONS 1
#include "utest.h"
#include "str.h"

UTEST(str, vd_str_path_last_part__basic) 
{
    str l = str_lit("C:/Users\\User/Desktop\\file.txt");
    str r = str_lit("file.txt");

    ASSERT_TRUE(str_eq(vd_str_path_last_part(l), r));
}

UTEST(str, vd_str_path_last_part__no_path) 
{
    str l = str_lit("file.txt");
    str r = str_lit("file.txt");

    ASSERT_TRUE(str_eq(vd_str_path_last_part(l), r));
}