#version 450

uniform vec3 u_color;

layout (location=0) out vec3 out_color;

void main()
{
    out_color = u_color;
}
