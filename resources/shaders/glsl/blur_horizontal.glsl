//
//  Implementation based on the article "Efficient Gaussian blur with linear sampling"
//  http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
//  A version for MasterEffect Reborn, a standalone version, and a custom shader version for SweetFX
//  can be found at http://reshade.me/forum/shader-presentation/27-gaussian-blur-bloom-unsharpmask
//
//  Taken from the RetroArch project and modified for ES-DE.
//

#define HW 1.00

#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec2 positionAttrib;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 TEX0;
uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * vec4(positionAttrib.xy, 0.0, 1.0);
    TEX0.xy = TexCoord.xy;
}

#elif defined(FRAGMENT)

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out COMPAT_PRECISION vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

uniform COMPAT_PRECISION vec2 TextureSize;
uniform sampler2D Texture;
COMPAT_VARYING COMPAT_PRECISION vec4 TEX0;

// Compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(TextureSize, 1.0 / TextureSize) // Either TextureSize or InputSize.

void main()
{
    vec2 texcoord = vTexCoord;
    vec2 PIXEL_SIZE = vec2(SourceSize.z, SourceSize.w);

#if __VERSION__ < 130
    float sampleOffsets1 = 0.0;
    float sampleOffsets2 = 1.4347826;
    float sampleOffsets3 = 3.3478260;
    float sampleOffsets4 = 5.2608695;
    float sampleOffsets5 = 7.1739130;

    float sampleWeights1 = 0.16818994;
    float sampleWeights2 = 0.27276957;
    float sampleWeights3 = 0.11690125;
    float sampleWeights4 = 0.024067905;
    float sampleWeights5 = 0.0021112196;

    // Alpha is handled differently depending on the graphics driver, so set it explicitly to 1.0.
    vec4 color = COMPAT_TEXTURE(Source, texcoord);
    color = vec4(color.rgb, 1.0) * sampleWeights1;

    color += COMPAT_TEXTURE(Source, texcoord + vec2(sampleOffsets2 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights2;
    color += COMPAT_TEXTURE(Source, texcoord - vec2(sampleOffsets2 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights2;

    color += COMPAT_TEXTURE(Source, texcoord + vec2(sampleOffsets3 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights3;
    color += COMPAT_TEXTURE(Source, texcoord - vec2(sampleOffsets3 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights3;

    color += COMPAT_TEXTURE(Source, texcoord + vec2(sampleOffsets4 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights4;
    color += COMPAT_TEXTURE(Source, texcoord - vec2(sampleOffsets4 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights4;

    color += COMPAT_TEXTURE(Source, texcoord + vec2(sampleOffsets5 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights5;
    color += COMPAT_TEXTURE(Source, texcoord - vec2(sampleOffsets5 * HW * PIXEL_SIZE.x, 0.0)) *
             sampleWeights5;
#else

    float sampleOffsets[5] = float[5](0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130);
    float sampleWeights[5] =
        float[5](0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196);

    vec4 color = COMPAT_TEXTURE(Source, texcoord) * sampleWeights[0];
    for (int i = 1; i < 5; i++) {
        color +=
            COMPAT_TEXTURE(Source, texcoord + vec2(sampleOffsets[i] * HW * PIXEL_SIZE.x, 0.0)) *
            sampleWeights[i];
        color +=
            COMPAT_TEXTURE(Source, texcoord - vec2(sampleOffsets[i] * HW * PIXEL_SIZE.x, 0.0)) *
            sampleWeights[i];
    }
#endif

    FragColor = vec4(color);
}
#endif
