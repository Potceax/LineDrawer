#version 430

in gData
{
    vec4 color;
}gs_out;

out vec4 outColor;

void main()
{
    outColor = gs_out.color;
}