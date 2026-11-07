#version 400

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 fragmentTexCoord;
in vec4 vertexColor;

uniform vec3 materialAmbient;
uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;
uniform int useLighting;
uniform sampler2D diffuseTexture;
uniform int useTexture;

out vec4 outputColor;

void main() {
    vec4 texel = useTexture == 1
        ? texture(diffuseTexture, fragmentTexCoord)
        : vec4(1.0);
    vec3 baseColor = vertexColor.rgb * texel.rgb;
    if (useLighting == 0) {
        outputColor = vec4(baseColor, vertexColor.a * texel.a);
        return;
    }

    vec3 normalDirection = normalize(fragmentNormal);
    vec3 lightDirection = normalize(vec3(0.3, 0.5, 1.0));
    vec3 viewDirection = normalize(-fragmentPosition);
    float diffuseStrength = max(dot(normalDirection, lightDirection), 0.0);
    float specularStrength = pow(max(dot(viewDirection,
                                         reflect(-lightDirection, normalDirection)),
                                     0.0),
                                materialShininess);
    vec3 litColor = baseColor * (materialAmbient + materialDiffuse * diffuseStrength)
        + materialSpecular * specularStrength;
    outputColor = vec4(litColor, vertexColor.a * texel.a);
}
