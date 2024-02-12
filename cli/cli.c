#include <stdio.h>
#include "lua.h"
#define VD_INTERNAL_SOURCE_FILE 1
#include "lauxlib.h"
#include "lualib.h"
#include "sys.h"

static struct {
    lua_State  *l;
    str         exec_path;
    Arena       a;
} G;

int main(int argc, char const *argv[])
{
    G.a = arena_new(4096, vd_memory_get_system_allocator());
    G.l = luaL_newstate();
    luaL_openlibs(G.l);

    G.exec_path = vd_get_exec_path(&G.a);

    lua_close(G.l);
    return 0;
}
