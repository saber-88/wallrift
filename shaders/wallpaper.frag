precision mediump float;

varying vec2 vTex;
varying vec2 vOldTex;
uniform sampler2D u_old_tex;
uniform sampler2D u_new_tex;
uniform float u_progress;
void main() {
    float t = smoothstep(0.0, 1.0, u_progress);
    vec4 oldColor = texture2D(u_old_tex, vOldTex);
    vec4 newColor = texture2D(u_new_tex, vTex);
    gl_FragColor = mix(oldColor, newColor, t);
}
