#define VD_INTERNAL_SOURCE_FILE 1
#include "r/types.h"

#include "cglm/affine.h"

void vd_r_generate_sphere_data(
    VD_R_Vertex **vertices,
    int *num_vertices,
    unsigned int **indices,
    int *num_indices,
    float radius,
    int segments,
    int rings,
    VD_Allocator *allocator)
{
    *num_vertices = (segments + 1) * (rings + 1);
    *num_indices = segments * rings * 6;

    *vertices = (VD_R_Vertex*)allocator->proc_alloc(
        0,
        0,
        sizeof(VD_R_Vertex) * *num_vertices,
        allocator->c);
    *indices = (unsigned int*)allocator->proc_alloc(
        0,
        0,
        sizeof(unsigned int) * *num_indices,
        allocator->c);

    int v_idx = 0;
    int i_idx = 0;

    for (int lat = 0; lat <= segments; ++lat) {
        float theta = (float)lat * GLM_PI / (float)segments;
        float sin_theta = sinf(theta);
        float cos_theta = cosf(theta);

        for (int lon = 0; lon <= rings; ++lon) {
            float phi = (float)lon * 2.0f * GLM_PI / (float)rings;
            float sin_phi = sinf(phi);
            float cos_phi = cosf(phi);

            vec3 position = {
                radius * sin_theta * cos_phi,
                radius * cos_theta,
                radius * sin_theta * sin_phi,
            };

            VD_R_Vertex vertex = {
                .uv_x = (float)lon / (float)rings,
                .uv_y = (float)lat / (float)segments,
                .color = { 1.0f, 1.0f, 1.0f, 1.0f }, 
            };

            glm_vec3_copy(position, vertex.position);

            glm_vec3_normalize(position);
            glm_vec3_copy(position, vertex.normal);

            (*vertices)[v_idx++] = vertex;
        }
    }

    for (int lat = 0; lat <= segments; ++lat) {
        for (int lon = 0; lon <= rings; ++lon) {
            int first = (lat * (rings + 1)) + lon;
            int second = first + rings + 1;

            (*indices)[i_idx++] = first;
            (*indices)[i_idx++] = second;
            (*indices)[i_idx++] = first + 1;

            (*indices)[i_idx++] = second;
            (*indices)[i_idx++] = second + 1;
            (*indices)[i_idx++] = first + 1;
        }
    }
}

void vd_r_generate_cube_data(
    VD_R_Vertex **vertices,
    int *num_vertices,
    unsigned int **indices,
    int *num_indices,
    vec3 extents,
    VD_Allocator *allocator)
{
    *num_vertices = 24;
    *num_indices = 36;

    *vertices = (VD_R_Vertex*)allocator->proc_alloc(
        0,
        0,
        sizeof(**vertices) * *num_vertices,
        allocator->c);
    *indices = (unsigned int*)allocator->proc_alloc(
        0,
        0,
        sizeof(**indices) * *num_indices,
        allocator->c);

    float hw = extents[0] / 2.0f;
    float hh = extents[1] / 2.0f;
    float hd = extents[2] / 2.0f;

    VD_R_Vertex cube_vertices[24] = {
        // Front face
        {{-hw, -hh,  hd}, 0.0f, { 0.0f,  0.0f,  1.0f}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // Bottom-left
        {{ hw, -hh,  hd}, 1.0f, { 0.0f,  0.0f,  1.0f}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // Bottom-right
        {{ hw,  hh,  hd}, 1.0f, { 0.0f,  0.0f,  1.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // Top-right
        {{-hw,  hh,  hd}, 0.0f, { 0.0f,  0.0f,  1.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}}, // Top-left

        // Back face
        {{ hw, -hh, -hd}, 0.0f, { 0.0f,  0.0f, -1.0f}, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}}, // Bottom-left
        {{-hw, -hh, -hd}, 1.0f, { 0.0f,  0.0f, -1.0f}, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}}, // Bottom-right
        {{-hw,  hh, -hd}, 1.0f, { 0.0f,  0.0f, -1.0f}, 1.0f, {1.0f, 0.0f, 0.0f, 1.0f}}, // Top-right
        {{ hw,  hh, -hd}, 0.0f, { 0.0f,  0.0f, -1.0f}, 1.0f, {1.0f, 0.0f, 0.0f, 1.0f}}, // Top-left

        // Left face
        {{-hw, -hh, -hd}, 0.0f, {-1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}}, // Bottom-left
        {{-hw, -hh,  hd}, 1.0f, {-1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}}, // Bottom-right
        {{-hw,  hh,  hd}, 1.0f, {-1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f}}, // Top-right
        {{-hw,  hh, -hd}, 0.0f, {-1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f}}, // Top-left

        // Right face
        {{ hw, -hh,  hd}, 0.0f, { 1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}}, // Bottom-left
        {{ hw, -hh, -hd}, 1.0f, { 1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}}, // Bottom-right
        {{ hw,  hh, -hd}, 1.0f, { 1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 0.0f, 1.0f, 1.0f}}, // Top-right
        {{ hw,  hh,  hd}, 0.0f, { 1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 0.0f, 1.0f, 1.0f}}, // Top-left

        // Top face
        {{-hw,  hh,  hd}, 0.0f, { 0.0f,  1.0f,  0.0f}, 0.0f, {1.0f, 1.0f, 0.0f, 1.0f}}, // Bottom-left
        {{ hw,  hh,  hd}, 1.0f, { 0.0f,  1.0f,  0.0f}, 0.0f, {1.0f, 1.0f, 0.0f, 1.0f}}, // Bottom-right
        {{ hw,  hh, -hd}, 1.0f, { 0.0f,  1.0f,  0.0f}, 1.0f, {1.0f, 1.0f, 0.0f, 1.0f}}, // Top-right
        {{-hw,  hh, -hd}, 0.0f, { 0.0f,  1.0f,  0.0f}, 1.0f, {1.0f, 1.0f, 0.0f, 1.0f}}, // Top-left

        // Bottom face
        {{-hw, -hh, -hd}, 0.0f, { 0.0f, -1.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 1.0f, 1.0f}}, // Bottom-left
        {{ hw, -hh, -hd}, 1.0f, { 0.0f, -1.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 1.0f, 1.0f}}, // Bottom-right
        {{ hw, -hh,  hd}, 1.0f, { 0.0f, -1.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 1.0f, 1.0f}}, // Top-right
        {{-hw, -hh,  hd}, 0.0f, { 0.0f, -1.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 1.0f, 1.0f}}, // Top-left
    };

    // Copy vertices to the output buffer
    for (int i = 0; i < 24; i++) {
        (*vertices)[i] = cube_vertices[i];
    }

    // Define indices for 12 triangles (2 triangles per face, 6 faces)
    unsigned int cube_indices[36] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Top face
        16, 17, 18, 18, 19, 16,
        // Bottom face
        20, 21, 22, 22, 23, 20,
    };

    // Copy indices to the output buffer
    for (int i = 0; i < 36; i++) {
        (*indices)[i] = cube_indices[i];
    }
}

void *vd_r_generate_checkerboard(
    u32 even_color,
    u32 odd_color,
    int width,
    int height,
    size_t *size,
    VD_Allocator *allocator)
{
    *size = width * height * sizeof(even_color);

    void *result = (void*)allocator->proc_alloc(0, 0, *size, allocator->c);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            ((u32*)result)[y*width + x] = ((x % 2) ^ (y % 2)) ? even_color : odd_color;
        }
    }

    return result;
}

void vd_r_perspective(mat4 proj, float fov, float aspect, float znear, float zfar)
{
    glm_perspective(fov, aspect, znear, zfar, proj);

    mat4 correction = GLM_MAT4_IDENTITY_INIT;

    correction[1][1] = -1.0f;

    glm_mat4_mul(correction, proj, proj);
}
