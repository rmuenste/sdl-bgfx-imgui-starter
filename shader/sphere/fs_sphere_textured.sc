$input v_normal, v_texcoord0, v_color

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);

// Light direction (from above-right-front)
uniform vec4 u_lightDir;

void main()
{
    // Sample checkerboard texture
    vec4 tex_color = texture2D(s_texColor, v_texcoord0);

    // Simple diffuse lighting
    vec3 light_dir = normalize(u_lightDir.xyz);
    vec3 normal = normalize(v_normal);
    float ndotl = max(dot(normal, light_dir), 0.0);

    // Ambient + diffuse
    float ambient = 0.3;
    float diffuse = 0.7 * ndotl;
    float lighting = ambient + diffuse;

    // Combine texture, instance color, and lighting
    vec3 final_color = tex_color.rgb * v_color.rgb * lighting;

    gl_FragColor = vec4(final_color, v_color.a);
}
