attribute vec4 apos;
varying vec2 vTex;
varying vec2 vOldTex;

uniform float u_cursor;

uniform float u_img_width;
uniform float u_img_height;
uniform float u_old_img_width;
uniform float u_old_img_height;

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
      scaleX = screenAspect / imageAspect;
    } else {
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

  // old image UV — same logic, different dimensions
  float oldAspect = u_old_img_width / u_old_img_height;
  float oldScaleX = 1.0, oldScaleY = 1.0;
  if (u_old_img_width > u_view_width || u_old_img_height > u_view_height) {
      if (oldAspect > screenAspect) oldScaleX = screenAspect / oldAspect;
      else oldScaleY = oldAspect / screenAspect;
  }
  float oldBaseX = 0.5 * (1.0 - oldScaleX);
  float oldBaseY = 0.5 * (1.0 - oldScaleY);
  float oldOffset = 0.0;
  if (oldScaleX < 1.0) {
      float available = 1.0 - oldScaleX;
      oldOffset = (u_cursor - 0.5) * available;
  }
  vOldTex = vec2(apos.z * oldScaleX + oldBaseX + oldOffset, apos.w * oldScaleY + oldBaseY);

  gl_Position = vec4(apos.x , apos.y , 0.0 , 1.0);
}
