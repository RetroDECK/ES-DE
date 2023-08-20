//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  blur_vertical.glsl
//
//  Vertical gaussian blur.
//

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
uniform sampler2D textureSampler;
uniform float blurStrength;
in vec2 texCoord;
out vec4 FragColor;

// shaderFlags:
// 0x00000001 - Premultiplied alpha (BGRA)
// 0x00000002 - Font texture
// 0x00000004 - Post processing
// 0x00000008 - Clipping
// 0x00000010 - Screen rotated 90 or 270 degrees
// 0x00000020 - Rounded corners
// 0x00000040 - Rounded corners with no anti-aliasing

void main()
{
    vec4 color = vec4(0.0);
    float hstep = 0.0f;
    float vstep = 1.0f;
    vec2 tc;

    if (0x0u != (shaderFlags & 0x10u)) {
        // Screen rotated 90 or 270 degrees.
        tc = texCoord.yx;
    }
    else {
        tc = texCoord.xy;
    }

    // 9-tap filter.
    color += texture(textureSampler,
                     vec2(tc.x - 4.0 * blurStrength * hstep, tc.y - 4.0 * blurStrength * vstep)) *
             0.0162162162;
    color += texture(textureSampler,
                     vec2(tc.x - 3.0 * blurStrength * hstep, tc.y - 3.0 * blurStrength * vstep)) *
             0.0540540541;
    color += texture(textureSampler,
                     vec2(tc.x - 2.0 * blurStrength * hstep, tc.y - 2.0 * blurStrength * vstep)) *
             0.1216216216;
    color += texture(textureSampler,
                     vec2(tc.x - 1.0 * blurStrength * hstep, tc.y - 1.0 * blurStrength * vstep)) *
             0.1945945946;

    color += texture(textureSampler, vec2(tc.x, tc.y)) * 0.2270270270;

    color += texture(textureSampler,
                     vec2(tc.x + 1.0 * blurStrength * hstep, tc.y + 1.0 * blurStrength * vstep)) *
             0.1945945946;
    color += texture(textureSampler,
                     vec2(tc.x + 2.0 * blurStrength * hstep, tc.y + 2.0 * blurStrength * vstep)) *
             0.1216216216;
    color += texture(textureSampler,
                     vec2(tc.x + 3.0 * blurStrength * hstep, tc.y + 3.0 * blurStrength * vstep)) *
             0.0540540541;
    color += texture(textureSampler,
                     vec2(tc.x + 4.0 * blurStrength * hstep, tc.y + 4.0 * blurStrength * vstep)) *
             0.0162162162;

    FragColor = vec4(color.rgb, 1.0);
}
#endif
