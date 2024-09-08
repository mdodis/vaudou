#define VD_NET_IMPLEMENTATION
#define VD_INTERNAL_SOURCE_FILE 1
#include <assert.h>
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "sys.h"
#include "fmt.h"
#include "array.h"
#include "vd_net.h"
static struct {
    lua_State     *l;
    str            exec_path;
    Arena          a;
    dynarray str  *builtins;
} G;

int l_c_parse(lua_State *l)
{
    lua_pushnumber(G.l, 1);
	printf("OMW\n");
    return 1;
}

int l_net_ftp(lua_State *l)
{
    if (!lua_isnumber(l, 1)) {
        return 0;
    }
    int port_num = lua_tointeger(l, 1);

    vd_net_ftp(& (VD_NetFtp) {
        .port = port_num,
    });
    return 0;
}

int main(int argc, char const *argv[])
{
    G.a = arena_new(4096*2, vd_memory_get_system_allocator());
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
            vd_fmt_printf("\t%{stru32}\n", str_path_last_part(G.builtins[i]));
        }
        return 1;
    }

    str program = str_from_cstr(argv[1]);

    VD_str a = vd_snfmt(&G.a, "%{stru32}/%{stru32}\0", builtins_path, program);
    vd_fmt_printf("%{stru32}\n", a);

    lua_pushcfunction(G.l, l_c_parse);
    lua_setglobal(G.l, "l_c_parse");

    lua_pushcfunction(G.l, l_net_ftp);
    lua_setglobal(G.l, "l_net_ftp");
    if (luaL_dofile(G.l, a.data)) {
		vd_fmt_printf("ERROR\n");
		vd_fmt_printf("LUA: %{cstr}\n", lua_tostring(G.l, -1));
	}


    lua_close(G.l);
    return 0;
}
