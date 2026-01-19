$input a_position, a_normal, a_texcoord0, i_data0, i_data1, i_data2, i_data3, i_data4
$output v_normal, v_texcoord0, v_color

#include <bgfx_shader.sh>

void main()
{
    // Reconstruct 4x4 instance transform matrix from instance data
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

    // Transform position by instance matrix, then by view-projection
    vec4 world_pos = mul(model, vec4(a_position, 1.0));
    gl_Position = mul(u_viewProj, world_pos);

    // Transform normal by instance matrix (upper 3x3)
    // For uniform scale, we can use the model matrix directly
    mat3 normal_matrix = mtxFromCols(i_data0.xyz, i_data1.xyz, i_data2.xyz);
    v_normal = normalize(mul(normal_matrix, a_normal));

    // Pass through texcoords and instance color
    v_texcoord0 = a_texcoord0;
    v_color = i_data4;
}
