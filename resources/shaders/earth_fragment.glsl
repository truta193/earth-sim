#version 330 core

in vec3 pos;
uniform sampler2D earth_ocean_sampler;
uniform sampler2D earth_detail_sampler;
uniform sampler2D earth_chloro_sampler;
uniform sampler2D earth_normal_sampler;
uniform sampler2D earth_ocean_normal_sampler;

uniform vec3 sun_position;
uniform vec3 camera_pos;
uniform mat4 earth_model;

out vec4 frag_color;

vec2 sphere_to_uv(vec3 p) {
    p = normalize(p);
    float longitude = atan(p.x, -p.z);
    float latitude = asin(p.y);

    const float PI = 3.14159;
    float u = (longitude / PI + 1) / 2;
    float v = latitude / PI + 0.5;
    return vec2(u, v);
}


vec3 blend_rnm(vec3 n1, vec3 n2) {
    n1 += vec3(0, 0, 1);
    n2 += vec3(0, 0, 1);
    vec3 n = n1*dot(n1, n2)/n1.z - n2;
    if (n.z < 0) n = normalize(vec3(n.x, n.y, 0));
    return n;

}

vec3 triplanar_normal(vec3 pos_t, vec3 normal) {
    vec3 abs_normal = abs(normal);
    float scale = 20.0;
    vec3 blend_weight = clamp(pow(abs_normal, vec3(4,4,4)), 0.0, 1.0);
    blend_weight /= dot(blend_weight, vec3(1,1,1));

    vec2 uvX = pos.zy * scale;
    vec2 uvY = pos.xz * scale;
    vec2 uvZ = pos.xy * scale;

    vec3 tnormalX = normalize(texture(earth_ocean_normal_sampler, uvX).rgb);
    vec3 tnormalY = normalize(texture(earth_ocean_normal_sampler, uvY).rgb);
    vec3 tnormalZ = normalize(texture(earth_ocean_normal_sampler, uvZ).rgb);

    tnormalX = blend_rnm(vec3(normal.zy, abs_normal.x), tnormalX);
    tnormalY = blend_rnm(vec3(normal.xz, abs_normal.y), tnormalY);
    tnormalZ = blend_rnm(vec3(normal.xy, abs_normal.z), tnormalZ);

    vec3 axis_sign = sign(normal);
    tnormalX.z *= axis_sign.x;
    tnormalY.z *= axis_sign.y;
    tnormalZ.z *= axis_sign.z;

    vec3 outt = normalize(
        tnormalX.zyx * blend_weight.x +
        tnormalY.xzy * blend_weight.y +
        tnormalZ.xyz * blend_weight.z
    );

    return outt;
}

//  0 - 105 water
//106 - 255 land
void main() {

    vec3 color = vec3(0.0, 0.0, 0.0);
    vec2 sphere_uv = sphere_to_uv(pos);

    float height = texture(earth_detail_sampler, sphere_uv).a;

    vec3 norm;
    if (height < 106.0 / 255.0) {
        norm = triplanar_normal(pos, pos);
    } else {
        norm = normalize(pos + texture(earth_normal_sampler, sphere_uv).rgb);
    }

    norm = transpose(inverse(mat3(earth_model))) * norm;

    vec3 light_dir = normalize(sun_position - pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

    vec3 specular = vec3(0.0, 0.0, 0.0);

    if (height < 106.0 / 255.0) {

        vec3 deep_blue = vec3(0.004, 0.137, 0.39);
        vec3 shallow_blueD = vec3(0.094, 0.29, 0.47);
        vec3 shallow_blueL = vec3(0.1255, 0.75, 0.8);
        vec3 chloroW = vec3(0.627, 1.0, 0.74);
        vec3 chloroS = vec3(0.0, 0.275, 0.04);

        float shallow_height = texture(earth_ocean_sampler, sphere_uv).r;
        float chloro = texture(earth_chloro_sampler, sphere_uv).r;

        vec3 shallow_color = mix(shallow_blueD, shallow_blueL, shallow_height);
        color = mix(deep_blue, shallow_color, chloro * 0.4 + height);

        float chloro_str = pow(chloro, 32);
        vec3 chloro_color = mix(chloroW, chloroS, chloro_str);
        color = mix(color, chloro_color, pow(chloro, 16));

        if (diff > 0.0) {
            vec3 view_dir = normalize(camera_pos - pos);
            vec3 reflect_dir = reflect(-light_dir, norm);
            float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 8);
            specular = 0.8 * spec * vec3(1.0, 1.0, 1.0);
        }

    } else {
        color = texture(earth_detail_sampler, sphere_uv).rgb;
    };

    frag_color = vec4((diffuse + specular) * color, 1.0);
}