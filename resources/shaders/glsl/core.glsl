//  SPDX-License-Identifier: MIT
//
//  ES-DE
//  core.glsl
//
//  Core shader functionality.
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
    color.abgr = colorVertex.rgba;
}

// Fragment section of code:
#elif defined(FRAGMENT)

#ifdef GL_ES
precision highp float;
#endif

in vec2 position;
in vec2 texCoord;
in vec4 color;

uniform vec2 texSize;
uniform vec4 clipRegion;
uniform float brightness;
uniform float saturation;
uniform float opacity;
uniform float dimming;
uniform float cornerRadius;
uniform float reflectionsFalloff;
uniform uint shaderFlags;

uniform sampler2D textureSampler0;
uniform sampler2D textureSampler1;
out vec4 FragColor;

// shaderFlags:
// 0x00000001 - Premultiplied alpha (BGRA)
// 0x00000002 - Font texture
// 0x00000004 - Post processing
// 0x00000008 - Clipping
// 0x00000010 - Screen rotated 90 or 270 degrees
// 0x00000020 - Rounded corners
// 0x00000040 - Rounded corners with no anti-aliasing
// 0x00000080 - Convert pixel format

void main()
{
    // Discard any pixels outside the clipping region.
    if (0x0u != (shaderFlags & 0x8u)) {
        if (position.x < clipRegion.x)
            discard;
        else if (position.y < clipRegion.y)
            discard;
        else if (position.x > clipRegion.z)
            discard;
        else if (position.y > clipRegion.w)
            discard;
    }

    vec4 sampledColor;

    // Pixel format conversion is sometimes required as not all mobile GPUs support all
    // OpenGL operations in BGRA format.
    if (0x0u != (shaderFlags & 0x80u))
        sampledColor.bgra = texture(textureSampler0, texCoord);
    else
        sampledColor = texture(textureSampler0, texCoord);

    // Rounded corners.
    if (0x0u != (shaderFlags & 0x20u) || 0x0u != (shaderFlags & 0x40u)) {
        float cornerRadiusClamped = cornerRadius;
        // Don't go beyond half the width and height.
        if (cornerRadiusClamped > texSize.x / 2.0)
            cornerRadiusClamped = texSize.x / 2.0;
        if (cornerRadiusClamped > texSize.y / 2.0)
            cornerRadiusClamped = texSize.y / 2.0;

        vec2 center = position - texSize / 2.0;
        vec2 q = abs(center) - (vec2(texSize.x / 2.0, texSize.y / 2.0) - cornerRadiusClamped);
        float pixelDistance = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - cornerRadiusClamped;

        if (pixelDistance > 0.0) {
            discard;
        }
        else {
            float pixelValue;
            if (0x0u != (shaderFlags & 0x20u))
                pixelValue = 1.0 - smoothstep(-0.75, 0.5, pixelDistance);
            else
                pixelValue = 1.0;

            sampledColor.a *= pixelValue;
            sampledColor.rgb *= pixelValue;
        }
    }

    // Brightness.
    if (brightness != 0.0) {
        sampledColor.rgb /= sampledColor.a;
        sampledColor.rgb += 0.3 * brightness;
        sampledColor.rgb *= sampledColor.a;
    }

    // Saturation, except for font textures.
    if (saturation != 1.0 && 0x0u == (shaderFlags & 0x2u)) {
        vec3 grayscale;
        // Premultiplied textures are all in BGRA format.
        if (0x0u != (shaderFlags & 0x01u))
            grayscale = vec3(dot(sampledColor.bgr, vec3(0.114, 0.587, 0.299)));
        else
            grayscale = vec3(dot(sampledColor.rgb, vec3(0.299, 0.587, 0.114)));
        vec3 blendedColor = mix(grayscale, sampledColor.rgb, saturation);
        sampledColor = vec4(blendedColor, sampledColor.a);
    }

    // For fonts the alpha information is stored in the red channel.
    if (0x0u != (shaderFlags & 0x2u))
        sampledColor = vec4(1.0, 1.0, 1.0, sampledColor.r);

    // We need different color calculations depending on whether the texture contains
    // premultiplied alpha or straight alpha values.
    if (0x0u != (shaderFlags & 0x01u)) {
        sampledColor.rgb *= color.rgb;
        sampledColor *= color.a;
    }
    else {
        sampledColor *= color;
    }

    // Saturation for font textures.
    if (saturation != 1.0 && 0x0u != (shaderFlags & 0x2u)) {
        vec3 grayscale = vec3(dot(sampledColor.rgb, vec3(0.299, 0.587, 0.114)));
        vec3 blendedColor = mix(grayscale, sampledColor.rgb, saturation);
        sampledColor = vec4(blendedColor, sampledColor.a);
    }

    // When post-processing we drop the alpha channel to avoid strange issues with some
    // graphics drivers.
    if (0x0u != (shaderFlags & 0x4u))
        sampledColor.a = 1.0;

    // Opacity.
    if (opacity != 1.0) {
        if (0x0u == (shaderFlags & 0x01u))
            sampledColor.a *= opacity;
        else
            sampledColor *= opacity;
    }

    // Dimming.
    if (dimming != 1.0) {
        vec4 dimColor = vec4(dimming, dimming, dimming, 1.0);
        sampledColor *= dimColor;
    }

    // Reflections falloff.
    if (reflectionsFalloff > 0.0)
        sampledColor.argb *= mix(0.0, 1.0, reflectionsFalloff - position.y) / reflectionsFalloff;

    FragColor = sampledColor;
}
#endif
