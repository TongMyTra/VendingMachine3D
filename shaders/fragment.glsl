#version 330 core
in vec3 vObjectPosition;
in vec3 vNormal;
in vec3 vWorldPosition;

uniform bool uFloor;
uniform bool uReflectionPass;
uniform sampler2D uReflectionTexture;
uniform vec2 uViewportSize;
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
    if (uFloor) {
        vec2 tile = fract((vWorldPosition.xz + vec2(19.0, 6.0)) / 2.2);
        vec2 edge = min(tile, 1.0 - tile);
        float grout = 1.0 - smoothstep(0.004, 0.011, min(edge.x, edge.y));
        float grain = fract(sin(dot(vWorldPosition.xz, vec2(127.1, 311.7))) * 43758.54);
        baseColor = mix(baseColor + (grain - 0.5) * 0.012, vec3(0.66, 0.69, 0.66), grout * 0.55);
        specular = pow(max(dot(normal, halfDirection), 0.0), 80.0) * 0.28;
    }
    vec3 litColor = baseColor * (ambient + diffuse * 0.43) + vec3(specular);
    litColor += uColor * uEmissive;
    if (uFloor && !uReflectionPass && !uWireframe) {
        vec2 uv = gl_FragCoord.xy / uViewportSize;
        vec2 blur = vec2(2.5) / uViewportSize;
        vec3 reflected = texture(uReflectionTexture, uv).rgb * 0.4;
        reflected += texture(uReflectionTexture, uv + vec2(blur.x, 0)).rgb * 0.15;
        reflected += texture(uReflectionTexture, uv - vec2(blur.x, 0)).rgb * 0.15;
        reflected += texture(uReflectionTexture, uv + vec2(0, blur.y)).rgb * 0.15;
        reflected += texture(uReflectionTexture, uv - vec2(0, blur.y)).rgb * 0.15;
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);
        litColor = mix(litColor, reflected, 0.12 + 0.12 * fresnel);
    }
    if (uWireframe) litColor = vec3(0.12, 0.36, 0.29);
    FragColor = vec4(litColor, uOpacity);
}
