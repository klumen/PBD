#version 430 core

struct Material
{
    vec3 diffuse;
    vec3 specular;
    float Ns;
};

struct DirLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
vec3 calculate_direction_light(DirLight light, vec3 normal, vec3 viewDir);

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D depthTexture;
uniform sampler2D thicknessTexture;
uniform sampler2D backgroundDepthTex;
uniform sampler2D backgroundTex;

uniform mat4 view;
uniform mat4 projectionInv;

uniform DirLight dirLight;
uniform Material material;

vec3 uv_to_view(vec2 coord)
{
    float z = texture(depthTexture, coord).r;
    vec4 clipPos = vec4((2.0 * coord - vec2(1.0)), z, 1.0);
    vec4 viewPos = projectionInv * clipPos;
    
    return viewPos.xyz / viewPos.w;
}

void main()
{
    float backgroundDepth = texture(backgroundDepthTex, texCoord).r;
    float depth = texture(depthTexture, texCoord).r;

    if (depth >= 1.0 || depth <= -1.0 || (depth * 0.5 + 0.5 > backgroundDepth))
    {
        fragColor = texture(backgroundTex, texCoord);
        return;
    }

    vec2 texSize = 1.0 / textureSize(depthTexture, 0);
    vec3 viewPos = uv_to_view(texCoord);

    vec3 ddu = uv_to_view(texCoord + vec2(texSize.x, 0.0)) - viewPos;
    vec3 bddu = viewPos - uv_to_view(texCoord - vec2(texSize.x, 0.0));
    if (abs(ddu.z) > abs(bddu.z))
        ddu = bddu;

    vec3 ddv = uv_to_view(texCoord + vec2(0.0, texSize.y)) - viewPos;
    vec3 bddv = viewPos - uv_to_view(texCoord - vec2(0.0, texSize.y));
    if (abs(ddv.z) > abs(bddv.z))
        ddv = bddv;

    vec3 normal = normalize(cross(ddu, ddv));

    // vec2 texScale = vec2(0.75, 1.0);
	// float refractScale = 1.33 * 0.025;
	// refractScale *= smoothstep(0.1, 0.4, worldPos.y);
	// vec2 refractCoord = texcoord + normal.xy * refractScale * texScale;
    float thickness = max(texture(thicknessTexture, texCoord).r, 0.3);
    vec3 transmission = exp(-(vec3(1.0) - material.diffuse) * thickness);
    vec3 refractColor = texture(backgroundTex, texCoord).xyz * transmission;

    vec3 viewDir = -normalize(viewPos);
    fragColor = vec4(calculate_direction_light(dirLight, normal, viewDir) + refractColor, 1.0);
}

vec3 calculate_direction_light(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize((view * vec4(-light.direction, 0.0)).xyz);
    vec3 halfDir = normalize(lightDir + viewDir);

    vec3 ambient = light.ambient * material.diffuse;
    vec3 diffuse = light.diffuse * max(dot(lightDir, normal), 0.0) * material.diffuse;
    vec3 specular = light.specular * pow(max(dot(halfDir, normal), 0.0), material.Ns) * material.specular;
    
    return (ambient + diffuse + specular);
}