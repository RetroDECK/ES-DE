//
//  Phosphor shader - Copyright (C) 2011 caligari.
//
//  Ported by Hyllian.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//  Taken from the RetroArch project and modified for ES-DE.
//

// Vertex section of code:
#if defined(VERTEX)

#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 MVPMatrix;
in vec2 positionVertex;
in vec2 texCoordVertex;
uniform vec2 textureSize;
out vec2 texCoord;
out vec2 onex;
out vec2 oney;

#define SourceSize vec4(textureSize, 1.0 / textureSize)

void main()
{
    gl_Position = MVPMatrix * vec4(positionVertex, 0.0, 1.0);
    texCoord = texCoordVertex;
    onex = vec2(SourceSize.z, 0.0);
    oney = vec2(0.0, SourceSize.w);
}

// Fragment section of code:
#elif defined(FRAGMENT)

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 textureSize;
uniform float opacity;
uniform float saturation;
uniform sampler2D textureSampler;
in vec2 texCoord;
in vec2 onex;
in vec2 oney;
out vec4 FragColor;

#define SourceSize vec4(textureSize, 1.0 / textureSize)

#ifdef PARAMETER_UNIFORM
uniform float SPOT_WIDTH;
uniform float SPOT_HEIGHT;
uniform float COLOR_BOOST;
uniform float InputGamma;
uniform float OutputGamma;
#else
#define SPOT_WIDTH 0.9
#define SPOT_HEIGHT 0.75
#define COLOR_BOOST 1.45
#define InputGamma 2.4
#define OutputGamma 2.2
#endif

#define GAMMA_IN(color) pow(color, vec4(InputGamma))
#define GAMMA_OUT(color) pow(color, vec4(1.0 / OutputGamma))

#define TEX2D(coords) GAMMA_IN(texture(textureSampler, coords))

// Macro for weights computing.
#define WEIGHT(w)                                                                                  \
    if (w > 1.0)                                                                                   \
        w = 1.0;                                                                                   \
    w = 1.0 - w * w;                                                                               \
    w = w * w;

void main()
{
    vec2 coords = (texCoord * SourceSize.xy);
    vec2 pixel_center = floor(coords) + vec2(0.5, 0.5);
    vec2 texture_coords = pixel_center * SourceSize.zw;

    vec4 color = TEX2D(texture_coords);
    float dx = coords.x - pixel_center.x;

    float h_weight_00 = dx / SPOT_WIDTH;
    WEIGHT(h_weight_00);

    // Saturation.
    if (saturation != 1.0) {
        vec3 grayscale = vec3(dot(color.rgb, vec3(0.34, 0.55, 0.11)));
        vec3 blendedColor = mix(grayscale, color.rgb, saturation);
        color = vec4(blendedColor, color.a);
    }

    color *= vec4(h_weight_00, h_weight_00, h_weight_00, h_weight_00);

    // Get closest horizontal neighbour to blend.
    vec2 coords01;
    if (dx > 0.0) {
        coords01 = onex;
        dx = 1.0 - dx;
    }
    else {
        coords01 = -onex;
        dx = 1.0 + dx;
    }
    vec4 colorNB = TEX2D(texture_coords + coords01);

    // Saturation.
    if (saturation != 1.0) {
        vec3 grayscale = vec3(dot(colorNB.rgb, vec3(0.34, 0.55, 0.11)));
        vec3 blendedColor = mix(grayscale, colorNB.rgb, saturation);
        colorNB = vec4(blendedColor, colorNB.a);
    }

    float h_weight_01 = dx / SPOT_WIDTH;
    WEIGHT(h_weight_01);

    color = color + colorNB * vec4(h_weight_01);

    // Vertical blending.
    float dy = coords.y - pixel_center.y;
    float v_weight_00 = dy / SPOT_HEIGHT;
    WEIGHT(v_weight_00);
    color *= vec4(v_weight_00);

    // Get closest vertical neighbour to blend.
    vec2 coords10;
    if (dy > 0.0) {
        coords10 = oney;
        dy = 1.0 - dy;
    }
    else {
        coords10 = -oney;
        dy = 1.0 + dy;
    }
    colorNB = TEX2D(texture_coords + coords10);

    // Saturation.
    if (saturation != 1.0) {
        vec3 grayscale = vec3(dot(colorNB.rgb, vec3(0.34, 0.55, 0.11)));
        vec3 blendedColor = mix(grayscale, colorNB.rgb, saturation);
        colorNB = vec4(blendedColor, colorNB.a);
    }

    float v_weight_10 = dy / SPOT_HEIGHT;
    WEIGHT(v_weight_10);

    color = color + colorNB * vec4(v_weight_10 * h_weight_00, v_weight_10 * h_weight_00,
                                   v_weight_10 * h_weight_00, v_weight_10 * h_weight_00);
    colorNB = TEX2D(texture_coords + coords01 + coords10);
    color = color + colorNB * vec4(v_weight_10 * h_weight_01, v_weight_10 * h_weight_01,
                                   v_weight_10 * h_weight_01, v_weight_10 * h_weight_01);
    color *= vec4(COLOR_BOOST);

    vec4 colorTemp = clamp(GAMMA_OUT(color), 0.0, 1.0);
    FragColor = vec4(colorTemp.rgb, colorTemp.a * opacity);
}
#endif
