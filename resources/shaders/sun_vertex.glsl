#version 460 core

in vec4 pos;
uniform mat4 camera_proj;
uniform mat4 camera_view;
uniform mat4 sun_model;

void main() {
    gl_Position = camera_proj * camera_view * sun_model * vec4(pos.xyz, 1.0);
}