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
  
  float scaleX = u_view_width / u_img_width; // how much to zoom to match img widht with screen
  float scaleY = u_view_height / u_img_height; // how much to zoom to match img height with screen
  float scale = max(scaleX , scaleY); // select the maximum scale so that both axes gets covered

  float uvRangeX = u_view_width / (scale * u_img_width); // what fraction of img widht is visible on screen
  float uvRangeY = u_view_height / (scale * u_img_height); // what fraction of img height is visible on screen
  
  float startU = u_cursor * ( 1.0 - uvRangeX); // starting img u coordinate on screen
  float startV = 0.5 * ( 1.0 - uvRangeY); // starting img v coordinate on screen

  vTex = vec2(apos.z * uvRangeX + startU, apos.w * uvRangeY + startV);

  // --- OLD IMAGE (same logic) ---
  float oldScaleX = u_view_width  / u_old_img_width;
  float oldScaleY = u_view_height / u_old_img_height;
  float oldScale  = max(oldScaleX, oldScaleY);

  float oldUvRangeX = u_view_width  / (oldScale * u_old_img_width);
  float oldUvRangeY = u_view_height / (oldScale * u_old_img_height);

  float oldStartU = u_cursor * (1.0 - oldUvRangeX);
  float oldStartV = 0.5 * (1.0 - oldUvRangeY);

  vOldTex = vec2(
      apos.z * oldUvRangeX + oldStartU,
      apos.w * oldUvRangeY + oldStartV
  );

  gl_Position = vec4(apos.x , apos.y , 0.0 , 1.0);
}
