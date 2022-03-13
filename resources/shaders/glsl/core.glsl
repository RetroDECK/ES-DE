//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  core.glsl
//
//  Core shader functionality:
//  opacity, saturation, dimming and BGRA to RGBA conversion.
//

#if defined(VERTEX)
// Vertex section of code:

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

uniform mat4 MVPMatrix;
COMPAT_ATTRIBUTE vec2 positionAttrib;
COMPAT_ATTRIBUTE vec2 TexCoord;
COMPAT_ATTRIBUTE vec4 colorAttrib;
COMPAT_VARYING vec4 color;
COMPAT_VARYING vec2 texCoord;

void main(void)
{
    color.rgba = colorAttrib.abgr;
    texCoord = TexCoord;
    gl_Position = MVPMatrix * vec4(positionAttrib.xy, 0.0, 1.0);
}

#elif defined(FRAGMENT)
// Fragment section of code:

#ifdef GL_ES
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
#define COMPAT_TEXTURE texture2D
#define FragColor gl_FragColor
#endif

COMPAT_VARYING COMPAT_PRECISION vec4 color;
COMPAT_VARYING COMPAT_PRECISION vec4 color2;
COMPAT_VARYING COMPAT_PRECISION vec2 texCoord;
uniform COMPAT_PRECISION float opacity;
uniform COMPAT_PRECISION float saturation;
uniform COMPAT_PRECISION float dimming;
uniform int BGRAToRGBA;
uniform int font;
uniform int postProcessing;
uniform sampler2D myTexture;

void main()
{
    COMPAT_PRECISION vec4 sampledColor = COMPAT_TEXTURE(myTexture, texCoord);

    // For fonts the alpha information is stored in the red channel.
    if (font == 1)
        sampledColor = vec4(1.0f, 1.0f, 1.0f, sampledColor.r);

    sampledColor *= color;

    // When post-processing we drop the alpha channel to avoid strange issues
    // with some graphics drivers.
    if (postProcessing == 1)
        sampledColor.a = 1.0f;

    // Opacity.
    if (opacity != 1.0f)
        sampledColor.a = sampledColor.a * opacity;

    // Saturation.
    if (saturation != 1.0f) {
        COMPAT_PRECISION vec3 grayscale = vec3(dot(sampledColor.rgb, vec3(0.34f, 0.55f, 0.11f)));
        COMPAT_PRECISION vec3 blendedColor = mix(grayscale, sampledColor.rgb, saturation);
        sampledColor = vec4(blendedColor, sampledColor.a);
    }

    // Dimming
    if (dimming != 1.0f) {
        COMPAT_PRECISION vec4 dimColor = vec4(dimming, dimming, dimming, 1.0f);
        sampledColor *= dimColor;
    }

    // BGRA to RGBA conversion.
    if (BGRAToRGBA == 1)
        sampledColor = vec4(sampledColor.bgr, sampledColor.a);

    FragColor = sampledColor;
}
#endif
