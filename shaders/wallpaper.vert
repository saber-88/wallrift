attribute vec4 apos;
varying vec2 vTex;

uniform float u_cursor;

uniform float u_img_width;
uniform float u_img_height;

uniform float u_view_width;
uniform float u_view_height;

void main(){

  float screenAspect = u_view_width / u_view_height;
  float imageAspect  = u_img_width / u_img_height;

  float scaleX = 1.0;
  float scaleY = 1.0;

  // Only scale if image bigger
  if (u_img_width > u_view_width || u_img_height > u_view_height) {

    if (imageAspect > screenAspect) {
      // image wider → crop X
      scaleX = screenAspect / imageAspect;
    } else {
      // image taller → crop Y
      scaleY = imageAspect / screenAspect;
    }
  }

  // center
  float baseX = 0.5 * (1.0 - scaleX);
  float baseY = 0.5 * (1.0 - scaleY);

  // parallax ONLY if horizontal overflow exists
  float offset = 0.0;
  if (scaleX < 1.0) {
    float available = 1.0 - scaleX;
    offset = (u_cursor - 0.5) * available;
  }

  vTex = vec2(
    apos.z * scaleX + baseX + offset,
    apos.w * scaleY + baseY
  );

  gl_Position = vec4(apos.x , apos.y , 0.0 , 1.0);
}
