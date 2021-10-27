attribute vec4 vPosition;
attribute vec3 vNormal;
attribute vec2 vTexCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

uniform vec4 lightPosition;

varying vec3 fN;
varying vec3 fE;
varying vec3 fL;
varying vec2 fTexCoord;

void main (){
    vec4 eyePosition = modelViewMatrix * vPosition;

    fN = normalMatrix * vNormal;
    fL = lightPosition.xyz - eyePosition.xyz;
    fE = -eyePosition.xyz;
    fTexCoord = vTexCoord;

    gl_Position = projectionMatrix * eyePosition;
}
