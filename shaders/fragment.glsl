#version 330 core
in vec3 vObjectPosition;
in vec3 vNormal;
in vec3 vWorldPosition;

uniform bool uWireframe;
uniform bool uUseTexture;
uniform sampler2D uLabelTexture;
uniform vec3 uColor;
uniform vec3 uCameraPosition;
uniform vec3 uLightPosition;
uniform float uOpacity;
uniform float uEmissive;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vWorldPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float ambient = 0.55;
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float specular = pow(max(dot(normal, halfDirection), 0.0), 40.0) * 0.18;
    vec3 baseColor = uColor;
    if (uUseTexture) {
        vec2 uv = vec2(atan(vObjectPosition.z, vObjectPosition.x) / 6.2831853 + 0.5, vObjectPosition.y + 0.5);
        baseColor = texture(uLabelTexture, uv).rgb;
    }
    vec3 litColor = baseColor * (ambient + diffuse * 0.43) + vec3(specular);
    litColor += uColor * uEmissive;
    if (uWireframe) litColor = vec3(0.12, 0.36, 0.29);
    FragColor = vec4(litColor, uOpacity);
}
