#version 430

layout(location = 0) in vec2 pos;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 direction;
layout(location = 3) in uint isCurve;


out vData
{
    vec2 pos;
    vec4 color;
    vec2 direction;
    uint isCurve;
}vs_in;


void main()
{
    vs_in.pos = pos;
    vs_in.color = color;
    vs_in.direction = direction;
    vs_in.isCurve = isCurve;

    gl_Position = vec4(pos.xy, -0.5, 1.0);
}