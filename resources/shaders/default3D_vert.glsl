#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 ourColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 cam = projection * view;
    gl_Position = cam * model * vec4(aPos, 1.0);
    ourColor = aColor;
}
