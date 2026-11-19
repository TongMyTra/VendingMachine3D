#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform bool uSky;
uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vObjectPosition;
out vec3 vNormal;
out vec3 vWorldPosition;

void main() {
    vObjectPosition = aPosition;
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vec4 clip = uMVP * vec4(aPosition, 1.0);
    gl_Position = uSky ? clip.xyww : clip;
}
