//
// desaturate.glsl
//
// Desaturates textures such as game images.
// The uniform variable 'shaderFloat_0' sets the saturation intensity.
// Setting this to the value 0 results in complete desaturation (grayscale).
//

// Vertex section of code:
// -----------------------
#if defined(VERTEX)

varying vec2 vTexCoord;

void main(void)
{
   vTexCoord = gl_MultiTexCoord0.xy;
   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
#endif

// Fragment section of code:
// -------------------------
#ifdef FRAGMENT

uniform float shaderFloat_0 = 1.0;
uniform sampler2D myTexture;
varying vec2 vTexCoord;

void main()
{
    float saturation = shaderFloat_0;
    vec4 color = texture2D(myTexture, vTexCoord);
    vec3 grayscale = vec3(dot(color.rgb, vec3(0.2125, 0.7154, 0.0721)));

    vec3 blendedColor = mix(grayscale, color.rgb, saturation);
    gl_FragColor = vec4(blendedColor, color.a);
}

#endif