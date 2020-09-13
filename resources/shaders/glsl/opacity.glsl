//
// opacity.glsl
//
// Changes the opacity of textures.
// The uniform variable 'opacity' sets the opacity.
// Setting this to the value 0 results in an invisible texture.
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

uniform float opacity = 1.0;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    vec4 color = texture2D(myTexture, vTexCoord);

    gl_FragColor = vec4(color.rgb, color.a * opacity);
}

#endif
