#version 330 core
in vec3 vNormal;
in vec3 vWorldPosition;

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
    vec3 litColor = uColor * (ambient + diffuse * 0.43) + vec3(specular);
    litColor += uColor * uEmissive;
    FragColor = vec4(litColor, uOpacity);
}
