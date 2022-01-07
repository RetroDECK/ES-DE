//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  bgra_to_rgba.glsl
//
//  Convert from color model BGRA to RGBA.
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

uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    vec4 color = texture2D(myTexture, vTexCoord);
    gl_FragColor = vec4(color.bgra);
}

#endif
