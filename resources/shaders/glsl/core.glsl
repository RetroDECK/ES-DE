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
#else
#define COMPAT_VARYING varying
#endif

uniform mat4 MVPMatrix;
COMPAT_VARYING vec4 color;
COMPAT_VARYING vec2 texCoord;

void main(void)
{
    texCoord = gl_MultiTexCoord0.xy;
    color.rgba = gl_Color.abgr;
    gl_Position = MVPMatrix * gl_Vertex;
}

#elif defined(FRAGMENT)
// Fragment section of code:

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_TEXTURE texture2D
#endif

COMPAT_VARYING vec4 color;
COMPAT_VARYING vec2 texCoord;
uniform float opacity = 1.0f;
uniform float saturation = 1.0f;
uniform float dimming = 1.0f;
uniform int BGRAToRGBA = 0;
uniform sampler2D myTexture;

void main()
{
    vec4 color = COMPAT_TEXTURE(myTexture, texCoord) * color;

    // Opacity.
    if (opacity != 1.0f)
        color.a = color.a * opacity;

    // Saturation.
    if (saturation != 1.0f) {
        vec3 grayscale = vec3(dot(color.rgb, vec3(0.3f, 0.59f, 0.11f)));
        vec3 blendedColor = mix(grayscale, color.rgb, saturation);
        color = vec4(blendedColor, color.a);
    }

    // Dimming
    vec4 dimColor = vec4(dimming, dimming, dimming, 1.0f);
    color = vec4(color.rgba) * dimColor;

    // BGRA to RGBA conversion.
    if (BGRAToRGBA == 1)
        color = vec4(color.bgr, color.a);

    gl_FragColor = color;
}
#endif
