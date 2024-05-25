#version 460 core

layout(rgba8ui, binding = 0) uniform uimage2D height_map;
layout(std430, binding = 1) buffer vertices {
    vec4 data[];
};

vec2 sphere_to_uv(vec3 p) {
    p = normalize(p);
    float longitude = atan(p.x, -p.z);
    float latitude = asin(p.y);

    const float PI = 3.14159;
    float u = (longitude / PI + 1) / 2;
    float v = latitude / PI + 0.5;
    return vec2(u, v);
}

layout(local_size_x = 1, local_size_y = 1) in;
void main() {
    uint i = gl_GlobalInvocationID.x;
    ivec2 pixel_position = ivec2(sphere_to_uv(data[i].xyz)*vec2(8192, 4096));
    float height = imageLoad(height_map, pixel_position).a;

    //  0 - 105 water
    //106 - 255 land
    data[i] = data[i] + data[i]*clamp(height, 106, 255)/6000;
}