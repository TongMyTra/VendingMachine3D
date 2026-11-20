#version 330 core
in vec3 vObjectPosition;
in vec3 vNormal;
in vec3 vWorldPosition;

uniform mat4 uCampusFrame;
uniform bool uSky;
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

float skyNoise(vec2 p) {
    vec2 cell = floor(p), t = fract(p);
    t = t * t * (3.0 - 2.0 * t);
    float a = fract(sin(dot(cell, vec2(127.1, 311.7))) * 43758.5453);
    float b = fract(sin(dot(cell + vec2(1, 0), vec2(127.1, 311.7))) * 43758.5453);
    float c = fract(sin(dot(cell + vec2(0, 1), vec2(127.1, 311.7))) * 43758.5453);
    float d = fract(sin(dot(cell + vec2(1, 1), vec2(127.1, 311.7))) * 43758.5453);
    return mix(mix(a, b, t.x), mix(c, d, t.x), t.y);
}
void main() {
    if (uSky) {
        vec3 direction = normalize(vWorldPosition - uCameraPosition);
        float height = clamp(direction.y, 0.0, 1.0);
        vec3 sky = mix(vec3(0.87, 0.92, 0.95), vec3(0.49, 0.71, 0.88), pow(height, 0.6));
        vec2 p = direction.xz / max(direction.y + 0.38, 0.16) * 3.0;
        float clouds = skyNoise(p) * 0.60 + skyNoise(p * 2.4) * 0.28 + skyNoise(p * 5.5) * 0.12;
        float cloud = smoothstep(0.52, 0.74, clouds) * smoothstep(-0.03, 0.35, direction.y);
        FragColor = vec4(mix(sky, vec3(0.98, 0.98, 0.96), cloud * 0.8), 1.0);
        return;
    }
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(uLightPosition - vWorldPosition);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float ambient = 0.55;
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float specular = pow(max(dot(normal, halfDirection), 0.0), 40.0) * 0.18;
    vec3 baseColor = uColor;
    if (uUseTexture) {
        vec2 uv = vec2(0.5 - atan(vObjectPosition.z, vObjectPosition.x) / 6.2831853, vObjectPosition.y + 0.5);
        baseColor = texture(uLabelTexture, uv).rgb;
    }
    if (uFloor) {
        vec3 campusPosition = vec3(uCampusFrame * vec4(vWorldPosition, 1.0));
        bool brickPath = campusPosition.z > 0.8 && abs(campusPosition.x) < 9.0;
        vec2 grid = (campusPosition.xz + vec2(19.0, 6.0)) / 1.5;
        if (brickPath) {
            grid = campusPosition.xz / vec2(0.9, 0.45);
            grid.x += mod(floor(grid.y), 2.0) * 0.5;
            baseColor = vec3(0.64, 0.43, 0.33);
        }
        vec2 tile = fract(grid);
        vec2 edge = min(tile, 1.0 - tile);
        float grout = 1.0 - smoothstep(0.004, 0.011, min(edge.x, edge.y));
        float grain = fract(sin(dot(vWorldPosition.xz, vec2(127.1, 311.7))) * 43758.54);
        baseColor = mix(baseColor + (grain - 0.5) * 0.012, vec3(0.66, 0.69, 0.66), grout * 0.55);
        specular = pow(max(dot(normal, halfDirection), 0.0), 65.0) * 0.16;
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
        litColor = mix(litColor, reflected, 0.035 + 0.04 * fresnel);
    }
    if (uWireframe) litColor = vec3(0.12, 0.36, 0.29);
    FragColor = vec4(litColor, uOpacity);
}
