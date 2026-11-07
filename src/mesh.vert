#version 400

in vec3 position;
in vec3 normal;
in vec2 texCoord;
in vec4 color;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fragmentPosition;
out vec3 fragmentNormal;
out vec2 fragmentTexCoord;
out vec4 vertexColor;

void main() {
    vec4 worldPosition = model * vec4(position, 1.0);
    mat4 modelView = view * model;
    fragmentPosition = (view * worldPosition).xyz;
    fragmentNormal = mat3(transpose(inverse(modelView))) * normal;
    fragmentTexCoord = texCoord;
    vertexColor = color;
    gl_Position = projection * view * worldPosition;
}
