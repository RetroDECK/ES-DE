//
// desaturate.glsl
//
// Desaturates textures such as game images.
// The uniform variable 'saturation' sets the saturation intensity.
// Setting this to the value 0 results in complete desaturation (grayscale).
//

#if defined(VERTEX)
// Vertex section of code:

varying vec2 vTexCoord;

void main(void)
{
    vTexCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

#elif defined(FRAGMENT)
// Fragment section of code:

uniform float saturation = 1.0;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    vec4 color = texture2D(myTexture, vTexCoord);
    vec3 grayscale = vec3(dot(color.rgb, vec3(0.2125, 0.7154, 0.0721)));

    vec3 blendedColor = mix(grayscale, color.rgb, saturation);
    gl_FragColor = vec4(blendedColor, color.a);
}

#endif