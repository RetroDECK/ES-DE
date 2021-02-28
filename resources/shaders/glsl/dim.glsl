//
// dim.glsl
//
// Dims textures.
// The uniform variable 'dimValue' sets the amount of dimming.
// Setting this to the value 0 results in a completely black screen.
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

uniform float dimValue = 0.4;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    vec4 dimColor = vec4(dimValue, dimValue, dimValue, 1.0);
    vec4 color = texture2D(myTexture, vTexCoord);
    // Alpha is handled differently depending on the graphics driver, so set it explicitly to 1.0.
    color = vec4(color.rgb, 1.0) * dimColor;

    gl_FragColor = color;
}

#endif
