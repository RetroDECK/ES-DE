//
//  Implementation based on the article "Efficient Gaussian blur with linear sampling"
//  http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
//  A version for MasterEffect Reborn, a standalone version, and a custom shader version for SweetFX
//  can be found at http://reshade.me/forum/shader-presentation/27-gaussian-blur-bloom-unsharpmask
//
//  Taken from the RetroArch project and modified for ES-DE.
//

#define HW 1.00

// Vertex section of code:
#if defined(VERTEX)

uniform mat4 MVPMatrix;
in vec2 positionVertex;
in vec2 texCoordVertex;
out vec2 texCoord;

void main()
{
    gl_Position = MVPMatrix * vec4(positionVertex, 0.0, 1.0);
    texCoord = texCoordVertex;
}

// Fragment section of code:
#elif defined(FRAGMENT)

#ifdef GL_ES
precision mediump float;
#endif

uniform uint shaderFlags;
uniform vec2 textureSize;
uniform sampler2D textureSampler;
in vec2 texCoord;
out vec4 FragColor;

// shaderFlags:
// 0x00000001 - Premultiplied alpha (BGRA)
// 0x00000002 - Font texture
// 0x00000004 - Post processing
// 0x00000008 - Clipping
// 0x00000010 - Screen rotated 90 or 270 degrees

void main()
{
    vec4 SourceSize;
    vec2 PIXEL_SIZE;

    if (0x0u != (shaderFlags & 0x10u)) {
        SourceSize = vec4(textureSize.yx, 1.0 / textureSize.xy);
        PIXEL_SIZE = vec2(SourceSize.w, SourceSize.z);
    }
    else {
        SourceSize = vec4(textureSize.xy, 1.0 / textureSize.xy);
        PIXEL_SIZE = vec2(SourceSize.z, SourceSize.w);
    }

    float sampleOffsets[5] = float[5](0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130);
    float sampleWeights[5] =
        float[5](0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196);

    vec4 color = texture(textureSampler, texCoord) * sampleWeights[0];
    for (int i = 1; i < 5; i++) {
        color +=
            texture(textureSampler, texCoord + vec2(sampleOffsets[i] * HW * PIXEL_SIZE.x, 0.0)) *
            sampleWeights[i];
        color +=
            texture(textureSampler, texCoord - vec2(sampleOffsets[i] * HW * PIXEL_SIZE.x, 0.0)) *
            sampleWeights[i];
    }

    FragColor = vec4(color);
}
#endif
