//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  core.glsl
//
//  Core shader functionality:
//  opacity, saturation, dimming and BGRA to RGBA conversion.
//

// Vertex section of code:
#if defined(VERTEX)

uniform mat4 MVPMatrix;
in vec2 positionVertex;
in vec2 texCoordVertex;
in vec4 colorVertex;

out vec4 color;
out vec2 texCoord;

void main(void)
{
    gl_Position = MVPMatrix * vec4(positionVertex.xy, 0.0, 1.0);
    texCoord = texCoordVertex;
    color.rgba = colorVertex.abgr;
}

// Fragment section of code:
#elif defined(FRAGMENT)

#ifdef GL_ES
precision mediump float;
#endif

in vec4 color;
in vec2 texCoord;
uniform float opacity;
uniform float saturation;
uniform float dimming;
uniform uint shaderFlags;

uniform sampler2D textureSampler;
out vec4 FragColor;

// shaderFlags:
// 0x00000001 - BGRA to RGBA conversion
// 0x00000002 - Font texture
// 0x00000004 - Post processing

void main()
{
    vec4 sampledColor = texture(textureSampler, texCoord);

    // For fonts the alpha information is stored in the red channel.
    if (0u != (shaderFlags & 2u))
        sampledColor = vec4(1.0, 1.0, 1.0, sampledColor.r);

    sampledColor *= color;

    // When post-processing we drop the alpha channel to avoid strange issues
    // with some graphics drivers.
    if (0u != (shaderFlags & 4u))
        sampledColor.a = 1.0;

    // Opacity.
    if (opacity != 1.0)
        sampledColor.a = sampledColor.a * opacity;

    // Saturation.
    if (saturation != 1.0) {
        vec3 grayscale = vec3(dot(sampledColor.rgb, vec3(0.34, 0.55, 0.11)));
        vec3 blendedColor = mix(grayscale, sampledColor.rgb, saturation);
        sampledColor = vec4(blendedColor, sampledColor.a);
    }

    // Dimming
    if (dimming != 1.0) {
        vec4 dimColor = vec4(dimming, dimming, dimming, 1.0);
        sampledColor *= dimColor;
    }

    // BGRA to RGBA conversion.
    if (0u != (shaderFlags & 1u))
        sampledColor = vec4(sampledColor.bgr, sampledColor.a);

    FragColor = sampledColor;
}
#endif
