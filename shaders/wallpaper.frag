precision mediump float;

varying vec2 vTex;
varying vec2 vOldTex;
uniform sampler2D u_old_tex;
uniform sampler2D u_new_tex;
uniform float u_progress;
uniform int u_type;

void main() {
    
    float t = smoothstep(0.0, 1.0, u_progress);
    vec4 oldColor = texture2D(u_old_tex, vOldTex);
    vec4 newColor = texture2D(u_new_tex, vTex);
    if(u_type == 0){ // fade
      gl_FragColor = mix(oldColor, newColor, t);  
    }
    else if(u_type == 1){ // wipe
      float edge = 0.05;
      float pos = t * (1.0 + 2.0 * edge) - edge;
      float alpha = smoothstep(pos - edge, pos + edge, vTex.x);
      gl_FragColor = mix(newColor, oldColor, alpha);
    }
}
