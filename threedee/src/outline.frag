uniform sampler2D texture;

float offset = 1.0 / 32;

void main()
{
  vec2 v_texCoords = gl_TexCoord[0].xy;
  vec4 col = texture2D(texture, v_texCoords);

  if (col.a > 0) {
    gl_FragColor = vec4(1.0, 1.0, 1.0, 0.0);
  } else {
    float au = texture2D(texture, vec2(v_texCoords.x , v_texCoords.y - offset)).a;
    float ad = texture2D(texture, vec2(v_texCoords.x , v_texCoords.y + offset)).a;
    float al = texture2D(texture, vec2(v_texCoords.x - offset, v_texCoords.y)).a;
    float ar = texture2D(texture, vec2(v_texCoords.x + offset, v_texCoords.y)).a;

    if (au > 0 || ad > 0 || al > 0 || ar > 0)
      gl_FragColor = vec4(1.0, 1.0, 1.0, 1);
    else
      gl_FragColor = col;
  }
}
