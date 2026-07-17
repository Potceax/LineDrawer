#version 430

#ifndef MAX_VERTS
#define MAX_VERTS 100
#endif

#ifndef SSBO_BINDING
#define SSBO_BINDING 0
#endif

layout(lines) in;
layout(line_strip, max_vertices = MAX_VERTS) out;

// SSBO
layout(std430, binding = SSBO_BINDING) buffer directionBuffer
{
    vec2 direction;
}dirBuff;

uniform int numberOfParts;

in vData
{
    vec2 pos;
    vec4 color;
    vec2 direction;
    uint isCurve;
}vs_out[];

out gData
{
    vec4 color;
}gs_in;

void GenArc(in vec2 u, in vec2 v, in int isLeft)
{
    /** In general you need 3 points to create an arc:
      * p0 - previous point
      * p1 - current first
      * p2 - current last
    */

    // calculate directions of previous line and current
    vec2 d1 = -vs_out[1].direction; // negate to achieve theta angle 
    vec2 d2 = normalize(vs_out[1].pos - vs_out[0].pos);

    // calculate theta between directions
    //float theta = acos(dot(d1, d2)); // better use atan2
    float theta = atan(d1.x * d1.y - d1.y * d2.x, dot(d1, d2));

    // calculate distance (d) to origin (O) from midpoint of line (B+A/2)
    vec2 mid = (vs_out[1].pos + vs_out[0].pos) * 0.5;
    //float d = tan(theta) / length(mid - vs_out[1].pos);
    float d = length(mid - vs_out[1].pos) / tan(theta);

    // get normal vector to the current line
    vec2 norm = vec2(-d2.y, d2.x);

    // TODO: norm has to, well mirror itself if it faces wrong direction (line in GenArc v1)
    // ...;

    // cross product between current line dir and previous dir (for side selection)
    float side = d1.x * d2.y - d1.y * d2.x;
    if(side < 0.0){
        norm = -norm;
    }

    // calculate Origin point of circle
    vec2 origin = mid + norm * d;

    // and get the radius
    float rad = length(origin - vs_out[0].pos);


    // rad origin and theta (angle between tangent and chord) are already defined
    // ...

    // calculate angleStart and angleEnd ()
    float angleStart = atan(vs_out[0].pos.y - origin.y, vs_out[0].pos.x - origin.x);
    float angleEnd = atan(vs_out[1].pos.y - origin.y, vs_out[1].pos.x - origin.x);

    // calculate angle, map to  [-PI, PI]
    float sweep = fract(angleEnd - angleStart);
    while (sweep > 3.14159)  sweep -= 2.0 * 3.14159;
    while (sweep < -3.14159) sweep += 2.0 * 3.14159;

    // change the main angle of draw
    if(theta < 0.0)
    {
        if(sweep < 0.0)
            sweep += 2.0 * 3.14159;
    }
    else
    {
        if(sweep > 0.0)
            sweep -= 2.0 * 3.14159;
    }

    // now for each segment generate angle
    vec2 prevPos = vs_out[0].pos;
    vec2 pos = vs_out[0].pos;

    for(int segment = 0; segment < numberOfParts; ++segment)
    {
        float alfa = float(segment) / (numberOfParts - 1);

        float beta = mix(angleStart, angleEnd, alfa);
        beta = angleStart + sweep * alfa;

        // calculate angle offset
        //alfa += 3.14159 * 0.5;

        //pos = origin + vec2( (cos(beta)* u.x) - (sin(beta) * v.x), (cos(beta)* u.y) - (sin(beta) * v.y));

        pos = rad * vec2(cos(beta), sin(beta));
        pos = origin + pos;
        gs_in.color = vec4(0.0);
        gl_Position = vec4(pos, 0.0, 1.0);
        EmitVertex();

        // ssbo write (pos - prevPos) [This makes arc flicker, idk if its a good idea...]
        //dirBuff.direction = pos - prevPos;
        //prevPos = pos;
    }

    // add last segment
    //gs_in.color = vec4(0.0);
    //gl_Position = vec4(vs_out[1].pos, 0.0, 1.0);
    //EmitVertex();

}

void CreateLine()
{
    // 1. find middle and angle of line
    vec2 diff = vs_out[1].pos - vs_out[0].pos;
    vec2 u = diff * 0.5;
    u = normalize(u);
    vec2 v = vec2(-u.y, u.x);
    vec2 pastDir = -vs_out[1].direction; // from p1 to p0 

    // 2. check the directions for left handines, for 2D the Z is the float check
    vec3 checkDir = cross(vec3(pastDir, 0.0), vec3(normalize(diff), 0.0));

    // DEBUG draw (a chord - line from P0 to P1)
    // gs_in.color = vec4(1.0, 0.0, 0.0, 1.0);
    // gl_Position = vec4(vs_out[0].pos.xy, -0.0, 1.0);
    // EmitVertex();
    // gs_in.color = vec4(1.0,0.0, 0.0, 1.0);
    // gl_Position = vec4(vs_out[1].pos.xy, -0.0, 1.0);
    // EmitVertex();
    // EndPrimitive();

    if(vs_out[1].isCurve == 0)
    {
        gs_in.color = vec4(1.0, 0.0, 0.0, 1.0);
        gl_Position = vec4(vs_out[0].pos.xy, -0.0, 1.0);
        EmitVertex();
        gs_in.color = vec4(1.0,0.0, 0.0, 1.0);
        gl_Position = vec4(vs_out[1].pos.xy, -0.0, 1.0);
        EmitVertex();
    }
    else
    {
        if(checkDir.z > 0)
        {
            v.x = -v.x;
            v.y = -v.y;
            GenArc(u, v, 0);
        }
        else
        {
            GenArc(u, v, 1);
        }
    }

    EndPrimitive();
}

void main()
{
    CreateLine();
}