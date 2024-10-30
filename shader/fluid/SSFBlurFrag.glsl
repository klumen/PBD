#version 430 core

in vec2 texCoord;

layout (location = 0) out float filterThickness;

uniform sampler2D depthTexture;
uniform sampler2D thicknessTexture;
uniform vec2 blurDir;
uniform float filterRadius;
uniform float spatialScale;
uniform float rangeScale;

void main()
{
    float depth = texture(depthTexture, texCoord).r;
    if (depth >= 1.0 || depth <= -1.0)
    {
        gl_FragDepth = depth;
        return;
    }

    float thickness = texture(thicknessTexture, texCoord).r;
    // vec2 texSize = 1.0 / textureSize(depthTexture, 0);

    float sumDepth = 0.0, sumThickness = 0.0;
    float wsumDepth = 0.0, wsumThickness = 0.0;
    // for (float y = -filterRadius; y <= filterRadius; y += 1.0)
    // {
    //     for (float x = -filterRadius; x <= filterRadius; x += 1.0)
    //     {
    //         float sampleDepth = texture(depthTexture, texCoord + vec2(x, y) * texSize).r;
    //         float sampleThickness = texture(thicknessTexture, texCoord + vec2(x, y) * texSize).r;

    //         if (sampleDepth >= 1.0 || sampleDepth <= -1.0)
    //             continue;

    //         float r = length(vec2(x, y)) * spatialScale;
    //         float w = exp(-r * r);

    //         float r2 = (sampleDepth - depth) * rangeScale;
    //         float g = exp(-r2 * r2);

    //         sumDepth += sampleDepth * w * g;
    //         sumThickness += sampleThickness * w;
    //         wsumDepth += w * g;
    //         wsumThickness += w;
    //     }
    // }
    for (float x = -filterRadius; x <= filterRadius; x += 1.0)
    {
        float sampleDepth = texture(depthTexture, texCoord + x * blurDir).r;
        float sampleThickness = texture(thicknessTexture, texCoord + x * blurDir).r;

        if (sampleDepth >= 1.0 || sampleDepth <= -1.0)
            continue;

        float r = x * spatialScale;
        float w = exp(-r * r);

        float r2 = (sampleDepth - depth) * rangeScale;
        float g = exp(-r2 * r2);

        sumDepth += sampleDepth * w * g;
        sumThickness += sampleThickness * w;
        wsumDepth += w * g;
        wsumThickness += w;
    }

    if (wsumDepth > 0.0) sumDepth /= wsumDepth;
    if (wsumThickness > 0.0) sumThickness /= wsumThickness;

    gl_FragDepth = sumDepth;
    filterThickness = sumThickness;
}