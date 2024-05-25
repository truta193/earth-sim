#include "external/ametrine.h"

//Declaratii
void update_cam(am_camera *camera);
am_util_obj *sphere_face_create(am_vec3 normal, am_int32 resolution, am_int32 sub, am_vec2 start);
void compute_face(am_util_obj **mesh, am_vec3 normal, am_int32 resolution, am_int32 sub);

//Macro-uri, constante si variabile globale
am_camera main_cam;
am_float32 CAMERA_SPEED = 5000.0f;
//Numarul de fete ale cubului si rezolutia acestora (512 => 512 * 512 vertices)
#define MESH_COUNT 6
#define MESH_RESOLUTION 512
am_float32 sun_rotation_speed = 1.0f;
am_float32 earth_rotation_speed = 1.0f;

//Structura care contine informatii despre obiectul soare
typedef struct sun_t {
    am_util_obj *mesh[6];
    //vqs - Vertex Quaternion Scale -> matrici de translatie, rotatie si marire
    am_vqs vqs;
    am_id shader;
} sun_t;
sun_t sun = {0};

typedef struct earth_t {
    am_util_obj  *mesh[MESH_COUNT];
    am_vqs vqs;
    am_id detail_map;
    am_id ocean_map;
    am_id ocean_normal_map;
    am_id chlorophyll_map;
    am_id normal_map;
    am_id shader;
} earth_t;
earth_t earth = {0};

//Functie pentru a crea o fata a cubului unitatii; este facuta in asa fel incat sa poata imparti o fata in mai
//multe sub-fete, pentru viitoare optimizari
am_util_obj *sphere_face_create(am_vec3 normal, am_int32 resolution, am_int32 sub, am_vec2 start) {
    //Doua axe pentru a determina planul in care ne aflam
    am_vec3 axisA = am_vec3_create(normal.y, normal.z, normal.x);
    am_vec3 axisB = am_vec3_cross(normal, axisA);
    //Alocare de spatiu pentru liste, in stack
    am_vec3 *vertices = (am_vec3*) am_malloc(sizeof(am_vec3) * resolution * resolution);
    am_int32 *indices = (am_int32*) am_malloc(sizeof(am_int32) * (resolution - 1) * (resolution - 1) * 6);
    am_int32 indices_index = 0;

    for (am_int32 y = 0; y < resolution; y++) {
        for (am_int32 x = 0; x < resolution; x++) {
            am_int32 vertex_index = x + y * resolution;
            //Procentaj [0, 1] pe doua axe pentru determinarea pozitiei in mesh
            am_vec2 percent = am_vec2_scale(1.0f / ((am_float32)resolution - 1.0f),am_vec2_create(x, y));
            percent = am_vec2_scale(1.0f / (am_float32)sub, percent);
            percent = am_vec2_add(percent, start);
            //Aducerea in pozitia corecta pe cubul unitar
            am_vec3 axisA_scaled = am_vec3_scale((2 * percent.x - 1), axisA);
            am_vec3 axisB_scaled = am_vec3_scale((2 * percent.y - 1), axisB);
            am_vec3 point = am_vec3_add(normal, am_vec3_add(axisA_scaled, axisB_scaled));

            //Imprastierea punctelor pentru evitarea aglomerarii la colturi in momentul normalizarii
            am_float32 x2 = point.x * point.x;
            am_float32 y2 = point.y * point.y;
            am_float32 z2 = point.z * point.z;
            am_float32 px = point.x * sqrtf(1.0f - (y2 + z2) / 2 + (y2 * z2) / 3);
            am_float32 py = point.y * sqrtf(1.0f - (z2 + x2) / 2 + (z2 * x2) / 3);
            am_float32 pz = point.z * sqrtf(1.0f - (x2 + y2) / 2 + (x2 * y2) / 3);

            vertices[vertex_index] = am_vec3_norm(am_vec3_create(px, py, pz));

            if (x != resolution - 1 && y != resolution - 1) {
                indices[indices_index + 0] = vertex_index;
                indices[indices_index + 1] = vertex_index + resolution + 1;
                indices[indices_index + 2] = vertex_index + resolution;
                indices[indices_index + 3] = vertex_index;
                indices[indices_index + 4] = vertex_index + 1;
                indices[indices_index + 5] = vertex_index + resolution + 1;
                indices_index += 6;
            }
        }
    }

    am_util_obj *face_mesh = (am_util_obj*) am_malloc(sizeof(am_util_obj));

    //Initializarea listelor
    face_mesh->vertices = NULL;
    face_mesh->normals = NULL;
    face_mesh->texture_coords = NULL;
    face_mesh->obj_vertices = NULL;
    face_mesh->indices = NULL;
    am_dyn_array_init((void**)&face_mesh->vertices, sizeof(am_float32));
    am_dyn_array_init((void**)&face_mesh->normals, sizeof(am_float32));
    am_dyn_array_init((void**)&face_mesh->texture_coords, sizeof(am_float32));
    am_dyn_array_init((void**)&face_mesh->obj_vertices, sizeof(am_util_obj_vertex));
    am_dyn_array_init((void**)&face_mesh->indices, sizeof(am_int32));

    //Umplerea listelor
    for (am_int32 i = 0; i < resolution * resolution; i++) {
        am_dyn_array_push(face_mesh->vertices, vertices[i].x);
        am_dyn_array_push(face_mesh->vertices, vertices[i].y);
        am_dyn_array_push(face_mesh->vertices, vertices[i].z);
        am_dyn_array_push(face_mesh->vertices, 0.0f);

        //NOTE: Remove
        am_dyn_array_push(face_mesh->normals, normal.x);
        am_dyn_array_push(face_mesh->normals, normal.y);
        am_dyn_array_push(face_mesh->normals, normal.z);

    }
    for (am_int32 i = 0; i < (resolution - 1) * (resolution - 1) * 6; i++)
        am_dyn_array_push(face_mesh->indices, indices[i]);

    //Eliberarea memoriei
    am_free(vertices);
    am_free(indices);
    return face_mesh;
}

void compute_face(am_util_obj **mesh, am_vec3 normal, am_int32 resolution, am_int32 sub) {
    for (am_int32 y = 0; y < sub; y++){
        for (am_int32 x = 0; x < sub; x++) {
            mesh[x + y * sub] = sphere_face_create(normal, resolution, sub, am_vec2_scale(1.0f / (am_float32)sub,am_vec2_create(x, y)));
        }
    }
}


am_id render_pass;
am_id earth_vertices, earth_indices, earth_pipeline;
am_id camera_view, camera_proj, camera_pos;
am_id earth_normal_sampler;
am_id earth_ocean_sampler;
am_id earth_detail_sampler;
am_id earth_chlorophyll_sampler;
am_id earth_ocean_normal_sampler;
am_id earth_sun_pos;
am_id vertex_compute_shader, vertex_compute_pipeline, vertex_compute_storage;
am_id sun_model_mat, earth_model_mat;
am_id sun_vertices, sun_indices, sun_pipeline;

void init() {
    //Initializarea camerei si blocarea mouse-ului
    main_cam = am_camera_perspective();
    main_cam.transform.position = am_vec3_create(0.0f, 1.0f, 3.0f);
    am_platform_mouse_lock(true);

    //Crearea shaderelor
    earth.shader = amgl_shader_create((amgl_shader_info) {
        .num_sources = 2,
        .sources = (amgl_shader_source_info[]) {
            {.type = AMGL_SHADER_VERTEX, .path = "resources/shaders/earth_vertex.glsl"},
            {.type = AMGL_SHADER_FRAGMENT, .path = "resources/shaders/earth_fragment.glsl"}
        }
    });

    sun.shader = amgl_shader_create((amgl_shader_info) {
        .num_sources = 2,
        .sources = (amgl_shader_source_info[]) {
            {.type = AMGL_SHADER_VERTEX, .path = "resources/shaders/sun_vertex.glsl"},
            {.type = AMGL_SHADER_FRAGMENT, .path = "resources/shaders/sun_fragment.glsl"},
        }
    });

    //Crearea texturilor
    earth.detail_map = amgl_texture_create((amgl_texture_info) {
        .format = AMGL_TEXTURE_FORMAT_RGBA8,
        .min_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .mag_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .wrap_s = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .wrap_t = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .path = "resources/textures/color_and_height.png"
    });


    earth.normal_map = amgl_texture_create((amgl_texture_info) {
        .format = AMGL_TEXTURE_FORMAT_RGBA8,
        .min_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .mag_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .wrap_s = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .wrap_t = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .path = "resources/textures/normal.png"
    });

    earth.ocean_map = amgl_texture_create((amgl_texture_info) {
        .format = AMGL_TEXTURE_FORMAT_RGBA8,
        .min_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .mag_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .wrap_s = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .wrap_t = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .path = "resources/textures/ocean.png"
    });

    earth.chlorophyll_map = amgl_texture_create((amgl_texture_info) {
        .format = AMGL_TEXTURE_FORMAT_RGBA8,
        .min_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .mag_filter = AMGL_TEXTURE_FILTER_LINEAR,
        .wrap_s = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .wrap_t = AMGL_TEXTURE_WRAP_CLAMP_TO_EDGE,
        .path = "resources/textures/chlorophyll.png"
    });

    earth.ocean_normal_map = amgl_texture_create((amgl_texture_info) {
        .format = AMGL_TEXTURE_FORMAT_RGBA8,
        .min_filter = AMGL_TEXTURE_FILTER_NEAREST,
        .mag_filter = AMGL_TEXTURE_FILTER_NEAREST,
        .wrap_s = AMGL_TEXTURE_WRAP_REPEAT,
        .wrap_t = AMGL_TEXTURE_WRAP_REPEAT,
        .path = "resources/textures/ocean_normal.png"
    });

    //Crearea fetelor pentru cele doua corpuri ceresti
    am_uint64 t = am_platform_elapsed_time();
    compute_face(&earth.mesh[0], am_vec3_create(0.0f, 0.0f, -1.0f), MESH_RESOLUTION, 1);
    compute_face(&earth.mesh[1], am_vec3_create(0.0f, 0.0f, 1.0f), MESH_RESOLUTION, 1);
    compute_face(&earth.mesh[2], am_vec3_create(0.0f, 1.0f, 0.0f), MESH_RESOLUTION, 1);
    compute_face(&earth.mesh[3], am_vec3_create(0.0f, -1.0f, 0.0f), MESH_RESOLUTION, 1);
    compute_face(&earth.mesh[4], am_vec3_create(1.0f, 0.0f, 0.0f), MESH_RESOLUTION, 1);
    compute_face(&earth.mesh[5], am_vec3_create(-1.0f, 0.0f, 0.0f), MESH_RESOLUTION, 1);

    compute_face(&sun.mesh[0], am_vec3_create(0.0f, 0.0f, -1.0f), 8, 1);
    compute_face(&sun.mesh[1], am_vec3_create(0.0f, 0.0f, 1.0f), 8, 1);
    compute_face(&sun.mesh[2], am_vec3_create(0.0f, 1.0f, 0.0f), 8, 1);
    compute_face(&sun.mesh[3], am_vec3_create(0.0f, -1.0f, 0.0f), 8, 1);
    compute_face(&sun.mesh[4], am_vec3_create(1.0f, 0.0f, 0.0f), 8, 1);
    compute_face(&sun.mesh[5], am_vec3_create(-1.0f, 0.0f, 0.0f), 8, 1);

    //Crearea bufferelor pentru Pamant
    earth_vertices = amgl_vertex_buffer_create((amgl_vertex_buffer_info) {
        .usage = AMGL_BUFFER_USAGE_DYNAMIC,
        .size = am_dyn_array_get_size(earth.mesh[0]->vertices),
        .data = earth.mesh[0]->vertices
    });

    earth_indices = amgl_index_buffer_create((amgl_index_buffer_info) {
        .usage = AMGL_BUFFER_USAGE_DYNAMIC,
        .size = am_dyn_array_get_size(earth.mesh[0]->indices),
        .data = earth.mesh[0]->indices
    });

    //Crearea pipeline-ului
    earth_pipeline = amgl_pipeline_create((amgl_pipeline_info) {
        .raster = {
            .primitive = AMGL_PRIMITIVE_TRIANGLES,
            .shader_id = earth.shader,
            .face_culling = AMGL_FACE_CULL_BACK
        },
        .depth = {.func = AMGL_DEPTH_FUNC_LESS},
        .layout = {
            .num_attribs = 1,
            .attributes = (amgl_vertex_buffer_attribute[]) {
                {.format = AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT4, .offset = 0, .stride = 4 * sizeof(am_float32), .buffer_index = 0}
            }
        }
    });

    //Crearea bufferelor pentru Soare
    sun_vertices = amgl_vertex_buffer_create((amgl_vertex_buffer_info) {
        .usage = AMGL_BUFFER_USAGE_DYNAMIC,
        .size = am_dyn_array_get_size(sun.mesh[0]->vertices),
        .data = sun.mesh[0]->vertices
    });

    sun_indices = amgl_index_buffer_create((amgl_index_buffer_info) {
        .usage = AMGL_BUFFER_USAGE_DYNAMIC,
        .size = am_dyn_array_get_size(sun.mesh[0]->indices),
        .data = sun.mesh[0]->indices
    });

    //Crearea pipeline-ului pentru Soare
    sun_pipeline = amgl_pipeline_create((amgl_pipeline_info) {
        .raster = {
            .primitive = AMGL_PRIMITIVE_TRIANGLES,
            .shader_id = sun.shader,
            .face_culling = AMGL_FACE_CULL_BACK
        },
        .depth = {.func = AMGL_DEPTH_FUNC_LESS},
        .layout = {
            .num_attribs = 1,
            .attributes = (amgl_vertex_buffer_attribute[]) {
                {.format = AMGL_VERTEX_BUFFER_ATTRIBUTE_FLOAT4, .offset = 0, .stride = 4 * sizeof(am_float32), .buffer_index = 0}
            }
        }
    });

    //Uniforms
    camera_view = amgl_uniform_create((amgl_uniform_info) {
        .name = "camera_view",
        .type = AMGL_UNIFORM_TYPE_MAT4
    });

    camera_proj = amgl_uniform_create((amgl_uniform_info) {
        .name = "camera_proj",
        .type = AMGL_UNIFORM_TYPE_MAT4
    });

    camera_pos = amgl_uniform_create((amgl_uniform_info) {
        .name = "camera_pos",
        .type = AMGL_UNIFORM_TYPE_VEC3
    });

    earth_ocean_sampler = amgl_uniform_create((amgl_uniform_info) {
        .name = "earth_ocean_sampler",
        .type = AMGL_UNIFORM_TYPE_SAMPLER2D
    });

    earth_detail_sampler = amgl_uniform_create((amgl_uniform_info) {
        .name = "earth_detail_sampler",
        .type = AMGL_UNIFORM_TYPE_SAMPLER2D
    });

    earth_normal_sampler = amgl_uniform_create((amgl_uniform_info) {
        .name = "earth_normal_sampler",
        .type = AMGL_UNIFORM_TYPE_SAMPLER2D
    });

    earth_chlorophyll_sampler = amgl_uniform_create((amgl_uniform_info) {
        .name = "earth_chloro_sampler",
        .type = AMGL_UNIFORM_TYPE_SAMPLER2D
    });

    earth_ocean_normal_sampler = amgl_uniform_create((amgl_uniform_info) {
        .name = "earth_ocean_normal_sampler",
        .type = AMGL_UNIFORM_TYPE_SAMPLER2D
    });

    earth_sun_pos = amgl_uniform_create((amgl_uniform_info) {
        .name = "sun_position",
        .type = AMGL_UNIFORM_TYPE_VEC3
    });

    earth_model_mat = amgl_uniform_create((amgl_uniform_info) {
            .name = "earth_model",
            .type = AMGL_UNIFORM_TYPE_MAT4
    });

    sun_model_mat = amgl_uniform_create((amgl_uniform_info) {
            .name = "sun_model",
            .type = AMGL_UNIFORM_TYPE_MAT4
    });

    sun.vqs = am_vqs_create(
        am_vec3_create(0.0f, 0.0f, 0.0f),
        am_quat_create(0.0f,0.0f,0.0f,1.0f),
        am_vec3_create(1.0f, 1.0f, 1.0f)
        );

    earth.vqs = am_vqs_create(
            am_vec3_create(0.0f, 0.0f, 0.0f),
            am_quat_create(0.0f,0.0f,0.0f,1.0f),
            am_vec3_create(1.0f, 1.0f, 1.0f)
    );

    render_pass = amgl_render_pass_create((amgl_render_pass_info){0});

    //Compute shader
    vertex_compute_shader = amgl_shader_create((amgl_shader_info) {
        .num_sources = 1,
        .sources = (amgl_shader_source_info[]) {
            {.type = AMGL_SHADER_COMPUTE, .path = "resources/shaders/vertices_compute.glsl"}
        }
    });

    vertex_compute_pipeline = amgl_pipeline_create((amgl_pipeline_info) {
        .compute = {
            .compute_shader = vertex_compute_shader
        }
    });

    vertex_compute_storage = amgl_storage_buffer_create((amgl_storage_buffer_info) {
        .usage = AMGL_BUFFER_USAGE_DYNAMIC,
        .size = am_dyn_array_get_size(earth.mesh[0]->vertices),
        .data = earth.mesh[0]->vertices
    });

    amgl_bindings_info vertex_compute_binds = {
        .storage_buffers = {
            .size = sizeof(amgl_storage_buffer_bind_info),
            .info = (amgl_storage_buffer_bind_info[]) {
                {.storage_buffer_id = vertex_compute_storage, .binding = 1}
            }
        },
        .images = {
            .size = sizeof(amgl_texture_bind_info),
            .info = (amgl_texture_bind_info[]) {
                {.texture_id = earth.detail_map, .access = AMGL_BUFFER_ACCESS_READ_WRITE, .binding = 0}
            }
        }
    };

    amgl_bind_pipeline(vertex_compute_pipeline);
    for (am_int32 i = 0; i < MESH_COUNT; i++) {
        amgl_storage_buffer_update(vertex_compute_storage, (amgl_storage_buffer_update_info) {
            .size = am_dyn_array_get_size(earth.mesh[i]->vertices),
            .data = earth.mesh[i]->vertices,
            .offset = 0
        });
        amgl_set_bindings(&vertex_compute_binds);
        amgl_shader_compute_dispatch(MESH_RESOLUTION * MESH_RESOLUTION, 1, 1);

        am_int32 buffer_size;
        glGetBufferParameteriv(GL_SHADER_STORAGE_BUFFER, GL_BUFFER_SIZE, &buffer_size);
        am_float32 *temp = (am_float32*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffer_size, GL_MAP_READ_BIT);
        memcpy(earth.mesh[i]->vertices, temp, buffer_size);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
}

void update() {
    //verificarea tastaturii pentru tastele ce reprezinta comentzi
    if (am_platform_key_down(AM_KEYCODE_ESCAPE)) am_engine_quit();
    if (am_platform_key_down(AM_KEYCODE_LEFT_SHIFT) && am_platform_key_down(AM_KEYCODE_LEFT_ARROW)) sun_rotation_speed -= 0.05f;
    if (am_platform_key_down(AM_KEYCODE_LEFT_SHIFT) && am_platform_key_down(AM_KEYCODE_RIGHT_ARROW)) sun_rotation_speed += 0.05f;
    if (!am_platform_key_down(AM_KEYCODE_LEFT_SHIFT) && am_platform_key_down(AM_KEYCODE_LEFT_ARROW)) earth_rotation_speed -= 0.05f;
    if (!am_platform_key_down(AM_KEYCODE_LEFT_SHIFT) && am_platform_key_down(AM_KEYCODE_RIGHT_ARROW)) earth_rotation_speed += 0.05f;

    am_platform *platform = am_engine_get_subsystem(platform);
    am_window *main = am_packed_array_get_ptr(platform->windows, 1);

    //Preluarea informatiilor despre perspepctiva etc.
    update_cam(&main_cam);
    am_mat4 view = am_camera_get_view(&main_cam);
    am_mat4 proj = am_camera_get_proj(&main_cam, (am_int32)main->width, (am_int32)main->height);

    //Rotatiile corpurilor, in functie de timp
    am_float32 coef = 100000000.0f;
    am_float32 tm = (am_float32)am_platform_elapsed_time();
    sun.vqs.translation = am_vec3_create(20.0f * cosf(tm*sun_rotation_speed/coef), 0.0f, 20.0f * sinf(tm*sun_rotation_speed/coef));
    earth.vqs.rotation = am_quat_angle_axis(tm*earth_rotation_speed/coef, am_vec3_create(0.0f, 0.8f, 0.234f));

    amgl_bindings_info earth_binds = {
        .index_buffers = {
            .size = sizeof(amgl_index_buffer_bind_info),
            .info = &(amgl_index_buffer_bind_info) {
                .index_buffer_id = earth_indices
            }
        },
        .vertex_buffers = {
            .size = sizeof(amgl_vertex_buffer_bind_info),
            .info = (amgl_vertex_buffer_bind_info[]) {
                {.vertex_buffer_id = earth_vertices}
            }
        },
        .uniforms = {
            .size = 10 * sizeof(amgl_uniform_bind_info),
            .info = (amgl_uniform_bind_info[]) {
                {.uniform_id = camera_view, .data = view.elements},
                {.uniform_id = camera_proj, .data = proj.elements},
                {.uniform_id = earth_model_mat, .data = am_vqs_to_mat4(&earth.vqs).elements},
                {.uniform_id = camera_pos, .data = main_cam.transform.position.xyz},
                {.uniform_id = earth_ocean_sampler, .data = &earth.ocean_map, .binding = 0},
                {.uniform_id = earth_detail_sampler, .data = &earth.detail_map, .binding = 0},
                {.uniform_id = earth_chlorophyll_sampler, .data = &earth.chlorophyll_map, .binding = 2},
                {.uniform_id = earth_normal_sampler, .data = &earth.normal_map, .binding = 1},
                {.uniform_id = earth_ocean_normal_sampler, .data = &earth.ocean_normal_map, .binding = 2},
                {.uniform_id = earth_sun_pos, .data = sun.vqs.position.xyz}
            }
        }
    };

    amgl_bindings_info sun_binds = {
        .index_buffers = {
            .size = sizeof(amgl_index_buffer_bind_info),
            .info = &(amgl_index_buffer_bind_info) {
                .index_buffer_id = sun_indices
            }
        },
        .vertex_buffers = {
            .size = sizeof(amgl_vertex_buffer_bind_info),
            .info = (amgl_vertex_buffer_bind_info[]) {
                {.vertex_buffer_id = sun_vertices}
            }
        },
        .uniforms = {
            .size = 3 * sizeof(amgl_uniform_bind_info),
            .info = (amgl_uniform_bind_info[]) {
                {.uniform_id = camera_view, .data = view.elements},
                {.uniform_id = camera_proj, .data = proj.elements},
                {.uniform_id = sun_model_mat, .data = am_vqs_to_mat4(&sun.vqs).elements}
            }
        }
    };

    amgl_start_render_pass(render_pass);

    amgl_clear((amgl_clear_desc) {
        .color = {0.0f, 0.0f, 0.0f, 0.0f},
        .num = 2,
        .types = (amgl_clear_type[]){AMGL_CLEAR_COLOR, AMGL_CLEAR_DEPTH}
    });

    //Desenarea corpurilor
    amgl_bind_pipeline(earth_pipeline);
    amgl_set_bindings(&earth_binds);
    for (int i = 0; i < MESH_COUNT; i++) {
        amgl_vertex_buffer_update(earth_vertices, (amgl_vertex_buffer_update_info) {
            .size = am_dyn_array_get_size(earth.mesh[i]->vertices),
            .data = earth.mesh[i]->vertices,
            .offset = 0
        });
        amgl_draw(&(amgl_draw_info) {.start = 0, .count = am_dyn_array_get_count(earth.mesh[i]->indices)});
    }

    amgl_bind_pipeline(sun_pipeline);
    amgl_set_bindings(&sun_binds);
    for (int i = 0; i < 6; i++) {
        amgl_vertex_buffer_update(sun_vertices, (amgl_vertex_buffer_update_info) {
            .size = am_dyn_array_get_size(sun.mesh[i]->vertices),
            .data = sun.mesh[i]->vertices,
            .offset = 0
        });
        amgl_draw(&(amgl_draw_info) {.start = 0, .count = am_dyn_array_get_count(sun.mesh[i]->indices)});
    }

    amgl_end_render_pass(render_pass);
}

void am_shutdown() {

}

//Functia actualizeaza detaliile camerei prin care privim
void update_cam(am_camera *camera) {
    am_platform *platform = am_engine_get_subsystem(platform);
    am_float64 dt = platform->time.delta;
    am_vec2 dm = am_platform_mouse_get_delta();

    am_camera_offset_orientation(camera, -0.1f * dm.x, -0.1f * dm.y);

    am_vec3 vel = {0};
    if (am_platform_key_down(AM_KEYCODE_W)) vel = am_vec3_add(vel, am_camera_forward(camera));
    if (am_platform_key_down(AM_KEYCODE_S)) vel = am_vec3_add(vel, am_camera_backward(camera));
    if (am_platform_key_down(AM_KEYCODE_A)) vel = am_vec3_add(vel, am_camera_left(camera));
    if (am_platform_key_down(AM_KEYCODE_D)) vel = am_vec3_add(vel, am_camera_right(camera));
    if (am_platform_key_down(AM_KEYCODE_SPACE)) vel = am_vec3_add(vel, am_camera_up(camera));
    if (am_platform_key_down(AM_KEYCODE_LEFT_CONTROL)) vel = am_vec3_add(vel, am_camera_down(camera));

    camera->transform.position = am_vec3_add(camera->transform.position, am_vec3_scale((am_float32)(CAMERA_SPEED * dt), am_vec3_norm(vel)));
}

int main() {
    //Initializarea apicatiei
    am_engine_create((am_engine_info) {
            .init = init,
            .update = update,
            .shutdown = am_shutdown,
            .win_fullscreen = true,
            .win_height = 1080,
            .win_width = 1920,
            .vsync_enabled = false,
            .desired_fps = 60
    });

    while (am_engine_get_instance()->is_running) am_engine_frame();
    return 0;
}