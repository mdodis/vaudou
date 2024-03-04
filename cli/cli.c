#define VD_INTERNAL_SOURCE_FILE 1
#include <assert.h>
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "sys.h"
#include "fmt.h"
#include "array.h"

static struct {
    lua_State     *l;
    str            exec_path;
    Arena          a;
    dynarray str  *builtins;
} G;

int main(int argc, char const *argv[])
{
    G.a = arena_new(4096, vd_memory_get_system_allocator());
    G.l = luaL_newstate();
    luaL_openlibs(G.l);

    G.exec_path = vd_get_exec_path(&G.a);
    vd_fmt_printf("Executable path: %{stru32}\n", G.exec_path);

    str builtins_path = vd_snfmt(
        &G.a, 
        "%{stru32}/builtin/\0", 
        vd_str_chop_right_last_of(G.exec_path, '/'));
    vd_fmt_printf("Builtins path: %{stru32}\n", builtins_path);
    
    VD_SysUtilDir dir;
    assert(vd_sysutil_dir_open(builtins_path.data, &dir) == 0);

    // Popuplate builtins array
    array_init(G.builtins, vd_memory_get_system_allocator());

    VD_SysUtilFileInfo entry;
    while (vd_sysutil_dir_next(&dir, &entry) == 0) {
        str file_name = {entry.name, entry.name_len};
        if (str_ends_with(file_name, str_lit(".lua"))) {
            array_add(
                G.builtins, 
                vd_snfmt(&G.a, "%{stru32}%{stru32}\0", builtins_path, file_name));
        }
    }

    // Parse arguments
    if (argc < 2) {
        vd_fmt_printf("Not enough arguments\n");
        vd_fmt_printf("USAGE\n");
        vd_fmt_printf("vdcli <program>\n");

        for (int i = 0; i < array_len(G.builtins); ++i) {
            vd_fmt_printf("\t%{stru32}\n", G.builtins[i]);
        }
        return 1;
    }

    str program = str_from_cstr(argv[2]);

    lua_close(G.l);
    return 0;
}
