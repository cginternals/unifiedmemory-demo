#version 450

layout (points) in;
layout (line_strip, max_vertices = 2) out;

void main()
{
    vec4 origin = gl_in[0].gl_Position;
    
    gl_Position = vec4(-1.0, origin.yzw);

    EmitVertex();

    gl_Position = vec4(1.0, origin.yzw);

    EmitVertex();

    EndPrimitive();
}
