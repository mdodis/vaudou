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

void vd_r_perspective(mat4 proj, float fov, float aspect, float znear, float zfar)
{
    glm_perspective(fov, aspect, znear, zfar, proj);

    mat4 correction = GLM_MAT4_IDENTITY_INIT;

    correction[1][1] = -1.0f;

    glm_mat4_mul(correction, proj, proj);
}
