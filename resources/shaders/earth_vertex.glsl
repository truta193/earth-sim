#version 460 core

in vec4 position;
uniform mat4 camera_view;
uniform mat4 camera_proj;

uniform mat4 earth_model;
out vec3 pos;

void main() {
    gl_Position =  camera_proj * camera_view * earth_model * vec4(position.xyz, 1.0);
    pos = position.xyz;
}