//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  desaturate.glsl
//
//  Desaturates textures.
//  The uniform variable 'saturation' sets the saturation intensity.
//  Setting this to the value 0 results in complete desaturation (grayscale).
//

#if defined(VERTEX)
// Vertex section of code:

uniform mat4 MVPMatrix;
varying vec2 vTexCoord;

void main(void)
{
    vTexCoord = gl_MultiTexCoord0.xy;
    gl_Position = MVPMatrix * gl_Vertex;
}

#elif defined(FRAGMENT)
// Fragment section of code:

uniform float saturation = 1.0;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    vec4 color = texture2D(myTexture, vTexCoord);
    vec3 grayscale = vec3(dot(color.rgb, vec3(0.3, 0.59, 0.11)));
    vec3 blendedColor = mix(grayscale, color.rgb, saturation);

    gl_FragColor = vec4(blendedColor, color.a);
}

#endif
