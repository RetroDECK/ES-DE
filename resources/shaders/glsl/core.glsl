//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  core.glsl
//
//  Core shader functionality:
//  clipping, opacity, saturation, dimming and BGRA to RGBA conversion.
//

// Vertex section of code:
#if defined(VERTEX)

uniform mat4 MVPMatrix;
in vec2 positionVertex;
in vec2 texCoordVertex;
in vec4 colorVertex;

out vec2 position;
out vec2 texCoord;
out vec4 color;

void main(void)
{
    gl_Position = MVPMatrix * vec4(positionVertex.xy, 0.0, 1.0);
    position = positionVertex;
    texCoord = texCoordVertex;
    color.rgba = colorVertex.abgr;
}

// Fragment section of code:
#elif defined(FRAGMENT)

#ifdef GL_ES
precision mediump float;
#endif

in vec2 position;
in vec2 texCoord;
in vec4 color;

uniform vec4 clipRegion;
uniform float opacity;
uniform float saturation;
uniform float dimming;
uniform float reflectionsFalloff;
uniform uint shaderFlags;

uniform sampler2D textureSampler;
out vec4 FragColor;

// shaderFlags:
// 0x00000001 - BGRA to RGBA conversion
// 0x00000002 - Font texture
// 0x00000004 - Post processing
// 0x00000008 - Clipping

void main()
{
    // Discard any pixels outside the clipping region.
    if (0u != (shaderFlags & 8u)) {
        if (position.x < clipRegion.x)
            discard;
        else if (position.y < clipRegion.y)
            discard;
        else if (position.x > clipRegion.z)
            discard;
        else if (position.y > clipRegion.w)
            discard;
    }

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
        sampledColor = sampledColor.bgra;

    // Reflections falloff.
    if (reflectionsFalloff > 0.0)
        sampledColor.a = mix(sampledColor.a, sampledColor.a - reflectionsFalloff, texCoord.y);

    FragColor = sampledColor;
}
#endif
