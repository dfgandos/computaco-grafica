varying vec3 fN;
varying vec3 fE;
varying vec3 fL;
varying vec2 fTexCoord;

uniform vec4 ambientProduct;
uniform vec4 diffuseProduct;
uniform vec4 specularProduct;
uniform float shininess;

uniform sampler2D texColorMap;
uniform sampler2D texNormalMap;

void main (){
    vec3 N = normalize ( texture2D( texNormalMap, fTexCoord).rgb * 2.0 - 1.0);
    vec3 E = normalize(fE);
    vec3 L = normalize(fL);
    vec3 R = normalize(2.0 * dot(L, N) * N - L);

    float NdotL = dot (N, L);
    float Kd = max (NdotL , 0.0) ;
    float Ks = ( NdotL < 0.0) ? 0.0 : pow ( max ( dot (R, E), 0.0), shininess );

    vec4 diffuse = Kd * diffuseProduct;
    vec4 specular = Ks * specularProduct;
    vec4 ambient = ambientProduct;

    gl_FragColor = (ambient + diffuse + specular) * texture2D(texColorMap,fTexCoord);
    gl_FragColor.a = 1.0;
}
