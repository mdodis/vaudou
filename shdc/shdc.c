#include "glslang_c_interface.h"

int main(int argc, char const *argv[])
{
    glslang_initialize_process();
    glslang_finalize_process();
    return 0;
}
