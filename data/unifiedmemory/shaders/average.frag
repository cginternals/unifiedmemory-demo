#version 450

uniform vec3 u_color;

layout (location=0) out vec3 out_color;

void main()
{
    out_color = mix(u_color, vec3(1.0, 1.0, 1.0), 0.7);
}
