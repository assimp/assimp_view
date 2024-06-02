#include "contrib/sokol/sokol_app.h"
#include "contrib/sokol/sokol_gfx.h"
#include "contrib/sokol/sokol_time.h"
#include "contrib/sokol/sokol_glue.h"


#include "AssimpViewerApp.h"
#include "MainRenderView.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SDL.h>

#if defined(__APPLE__)
	#define SOKOL_METAL
#elif defined(_WIN32)
	#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
	#define SOKOL_GLES2
#else
	#define SOKOL_GLCORE33
#endif

#include "contrib/sokol/sokol_app.h"
#include "contrib/sokol/sokol_time.h"
#include "contrib/sokol/sokol_gfx.h"

#define MAX_BONES 64
#define MAX_BLEND_SHAPES 64

struct vec2 {
    float x, y;
};

struct vec3 {
    float x,y,z;
};

struct mesh_vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;
	float f_vertex_index;
};

struct skin_vertex {
	uint8_t bone_index[4];
	uint8_t bone_weight[4];
};

static sg_layout_desc mesh_vertex_layout;

static void initMeshVertexFormat() {
	mesh_vertex_layout.attrs[0].buffer_index = 0;
	mesh_vertex_layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;

	mesh_vertex_layout.attrs[1].buffer_index = 0;
	mesh_vertex_layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;

	mesh_vertex_layout.attrs[2].buffer_index = 0;
	mesh_vertex_layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2;

	mesh_vertex_layout.attrs[3].buffer_index = 0;
	mesh_vertex_layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT;
}

static sg_layout_desc skinned_mesh_vertex_layout;

static void initSkinnedMeshVertexFormat() {
	skinned_mesh_vertex_layout.attrs[0].buffer_index = 0;
	skinned_mesh_vertex_layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;

	skinned_mesh_vertex_layout.attrs[1].buffer_index = 0;
	skinned_mesh_vertex_layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT3;

	skinned_mesh_vertex_layout.attrs[2].buffer_index = 0;
	skinned_mesh_vertex_layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT2;

	skinned_mesh_vertex_layout.attrs[3].buffer_index = 0;
	skinned_mesh_vertex_layout.attrs[3].format = SG_VERTEXFORMAT_FLOAT;
}

struct quat {
    float x,y,z,w;
};

struct mat4 {
    float m11, m21, m31, m41;
    float m12, m22, m32, m42;
    float m13, m23, m33, m43;
    float m14, m24, m34, m44;
};

struct viewer_node_anim {
	float time_begin;
	float framerate;
	size_t num_frames;
	quat const_rot;
	vec3 const_pos;
	vec3 const_scale;
	quat *rot;
	vec3 *pos;
	vec3 *scale;
};

struct viewer_blend_channel_anim {
	float const_weight;
	float *weight;
};

struct viewer_anim {
	const char *name;
	float time_begin;
	float time_end;
	float framerate;
	size_t num_frames;

	viewer_node_anim *nodes;
	viewer_blend_channel_anim *blend_channels;
};

struct viewer_node {
	int32_t parent_index;

	mat4 geometry_to_node;
	mat4 node_to_parent;
	mat4 node_to_world;
	mat4 geometry_to_world;
	mat4 normal_to_world;
};

struct viewer_blend_channel {
	float weight;
};

struct viewer_mesh_part {
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	sg_buffer skin_buffer; // Optional

	size_t num_indices;
	int32_t material_index;
};

struct viewer_mesh {
	int32_t *instance_node_indices;
	size_t num_instances;

	viewer_mesh_part *parts;
	size_t num_parts;

	bool aabb_is_local;
	vec3 aabb_min;
	vec3 aabb_max;

	// Skinning (optional)
	bool skinned;
	size_t num_bones;
	int32_t bone_indices[MAX_BONES];
	mat4 bone_matrices[MAX_BONES];

	// Blend shapes (optional)
	size_t num_blend_shapes;
	sg_image blend_shape_image;
	int32_t blend_channel_indices[MAX_BLEND_SHAPES];
};

struct viewer_scene {
	viewer_node *nodes;
	size_t num_nodes;

	viewer_mesh *meshes;
	size_t num_meshes;

	viewer_blend_channel *blend_channels;
	size_t num_blend_channels;

	viewer_anim *animations;
	size_t num_animations;

	vec3 aabb_min;
	vec3 aabb_max;
};

struct viewer {
	viewer_scene scene;
	float anim_time;

	sg_shader shader_mesh_lit_static;
	sg_shader shader_mesh_lit_skinned;
	sg_pipeline pipe_mesh_lit_static;
	sg_pipeline pipe_mesh_lit_skinned;
	sg_image empty_blend_shape_image;

	mat4 world_to_view;
	mat4 view_to_clip;
	mat4 world_to_clip;

	float camera_yaw;
	float camera_pitch;
	float camera_distance;
	uint32_t mouse_buttons;
};

namespace AssimpViewer {

static constexpr char Tag[] = "OsreEdApp";

static void createTitleString(const std::string &assetName, std::string &titleString) {
    titleString.clear();
    titleString += "AssimpViewer!";

    titleString += " Asset: ";
    titleString += assetName;
}

AssimpViewerApp::AssimpViewerApp(int argc, char *argv[]) : 
        mTitle(),
        mWindow(nullptr),
        mSceneData() {
    // empty
}

AssimpViewerApp::~AssimpViewerApp() {
    // empty
}

const aiScene *AssimpViewerApp::importAssimp(const std::string &path) {
    return nullptr;
}

void *alloc_imp(size_t type_size, size_t count)
{
	void *ptr = malloc(type_size * count);
	if (!ptr) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(ptr, 0, type_size * count);
	return ptr;
}

void *alloc_dup_imp(size_t type_size, size_t count, const void *data)
{
	void *ptr = malloc(type_size * count);
	if (!ptr) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memcpy(ptr, data, type_size * count);
	return ptr;
}

#define alloc(m_type, m_count) (m_type*)alloc_imp(sizeof(m_type), (m_count))
#define alloc_dup(m_type, m_count, m_data) (m_type*)alloc_dup_imp(sizeof(m_type), (m_count), (m_data))

size_t min_sz(size_t a, size_t b) { return a < b ? a : b; }
size_t max_sz(size_t a, size_t b) { return b < a ? a : b; }
size_t clamp_sz(size_t a, size_t min_a, size_t max_a) { return min_sz(max_sz(a, min_a), max_a); }

struct viewer_node_anim {
	float time_begin;
	float framerate;
	size_t num_frames;
	quat const_rot;
	vec3 const_pos;
	vec3 const_scale;
	quat *rot;
	vec3 *pos;
	vec3 *scale;
};

struct viewer_blend_channel_anim {
	float const_weight;
	float *weight;
};

struct viewer_anim {
	const char *name;
	float time_begin;
	float time_end;
	float framerate;
	size_t num_frames;

	viewer_node_anim *nodes;
	viewer_blend_channel_anim *blend_channels;
};

struct viewer_node {
	int32_t parent_index;

	mat4 geometry_to_node;
	mat4 node_to_parent;
	mat4 node_to_world;
	mat4 geometry_to_world;
	mat4 normal_to_world;
};

struct viewer_blend_channel {
	float weight;
};

struct viewer_mesh_part {
	sg_buffer vertex_buffer;
	sg_buffer index_buffer;
	sg_buffer skin_buffer; // Optional

	size_t num_indices;
	int32_t material_index;
};

struct viewer_mesh {
	int32_t *instance_node_indices;
	size_t num_instances;

	viewer_mesh_part *parts;
	size_t num_parts;

	bool aabb_is_local;
	vec3 aabb_min;
	vec3 aabb_max;

	// Skinning (optional)
	bool skinned;
	size_t num_bones;
	int32_t bone_indices[MAX_BONES];
	mat4 bone_matrices[MAX_BONES];

	// Blend shapes (optional)
	size_t num_blend_shapes;
	sg_image blend_shape_image;
	int32_t blend_channel_indices[MAX_BLEND_SHAPES];
};

struct viewer_scene {

	viewer_node *nodes;
	size_t num_nodes;

	viewer_mesh *meshes;
	size_t num_meshes;

	viewer_blend_channel *blend_channels;
	size_t num_blend_channels;

	viewer_anim *animations;
	size_t num_animations;

	vec3 aabb_min;
	vec3 aabb_max;

};

struct viewer {

	viewer_scene scene;
	float anim_time;

	sg_shader shader_mesh_lit_static;
	sg_shader shader_mesh_lit_skinned;
	sg_pipeline pipe_mesh_lit_static;
	sg_pipeline pipe_mesh_lit_skinned;
	sg_image empty_blend_shape_image;

	mat4 world_to_view;
	mat4 view_to_clip;
	mat4 world_to_clip;

	float camera_yaw;
	float camera_pitch;
	float camera_distance;
	uint32_t mouse_buttons;
};

void read_node(viewer_node *vnode, aiNode *node)
{
/*	vnode->parent_index = node->parent ? node->parent->typed_id : -1;
	vnode->node_to_parent = ufbx_to_um_mat(node->node_to_parent);
	vnode->node_to_world = ufbx_to_um_mat(node->node_to_world);
	vnode->geometry_to_node = ufbx_to_um_mat(node->geometry_to_node);
	vnode->geometry_to_world = ufbx_to_um_mat(node->geometry_to_world);
	vnode->normal_to_world = ufbx_to_um_mat(ufbx_matrix_for_normals(&node->geometry_to_world));*/
}

/*sg_image pack_blend_channels_to_image(ufbx_mesh *mesh, ufbx_blend_channel **channels, size_t num_channels)
{
	// We pack the blend shape data into a 1024xNxM texture array where each texel
	// contains the vertex `Y*1024 + X` for blend shape `Z`.
	uint32_t tex_width = 1024;
	uint32_t tex_height_min = ((uint32_t)mesh->num_vertices + tex_width - 1) / tex_width;
	uint32_t tex_slices = (uint32_t)num_channels;

	// Let's make the texture size a power of two just to be sure...
	uint32_t tex_height = 1;
	while (tex_height < tex_height_min) {
		tex_height *= 2;
	}

	// NOTE: A proper implementation would probably compress the shape offsets to FP16
	// or some other quantization to save space, we use full FP32 here for simplicity.
	size_t tex_texels = tex_width * tex_height * tex_slices;
	vec4 *tex_data = alloc(vec4, tex_texels);

	// Copy the vertex offsets from each blend shape
	for (uint32_t ci = 0; ci < num_channels; ci++) {
		ufbx_blend_channel *chan = channels[ci];
		vec4 *slice_data = tex_data + tex_width * tex_height * ci;
	
		// Let's use the last blend shape if there's multiple blend phases as we don't
		// support it. Fortunately this feature is quite rarely used in practice.
		ufbx_blend_shape *shape = chan->keyframes.data[chan->keyframes.count - 1].shape;

		for (size_t oi = 0; oi < shape->num_offsets; oi++) {
			uint32_t ix = (uint32_t)shape->offset_vertices.data[oi];
			if (ix < mesh->num_vertices) {
				// We don't need to do any indexing to X/Y here as the memory layout of
				// `slice_data` pixels is the same as the linear buffer would be.
				slice_data[ix].xyz = ufbx_to_um_vec3(shape->position_offsets.data[oi]);
			}
		}
	}

	// Upload the combined blend offset image to the GPU
	sg_image image = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_ARRAY,
		.width = (int)tex_width,
		.height = (int)tex_height,
		.num_slices = tex_slices,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
		.data.subimage[0][0] = { tex_data, tex_texels * sizeof(um_vec4) },
	});

	free(tex_data);

	return image;
}
*/
void read_mesh(viewer_mesh *vmesh, aiMesh *mesh)
{
	// Count the number of needed parts and temporary buffers
	size_t max_parts = 0;
	size_t max_triangles = 0;

	// We need to render each material of the mesh in a separate part, so let's
	// count the number of parts and maximum number of triangles needed.
	/*for (size_t pi = 0; pi < mesh->material_parts.count; pi++) {
		ufbx_mesh_part *part = &mesh->material_parts.data[pi];
		if (part->num_triangles == 0) continue;
		max_parts += 1;
		max_triangles = max_sz(max_triangles, part->num_triangles);
	}

	// Temporary buffers
	size_t num_tri_indices = mesh->max_face_triangles * 3;
	uint32_t *tri_indices = alloc(uint32_t, num_tri_indices);
	mesh_vertex *vertices = alloc(mesh_vertex, max_triangles * 3);
	skin_vertex *skin_vertices = alloc(skin_vertex, max_triangles * 3);
	skin_vertex *mesh_skin_vertices = alloc(skin_vertex, mesh->num_vertices);
	uint32_t *indices = alloc(uint32_t, max_triangles * 3);

	// Result buffers
	viewer_mesh_part *parts = alloc(viewer_mesh_part, max_parts);
	size_t num_parts = 0;

	// In FBX files a single mesh can be instanced by multiple nodes. ufbx handles the connection
	// in two ways: (1) `ufbx_node.mesh/light/camera/etc` contains pointer to the data "attribute"
	// that node uses and (2) each element that can be connected to a node contains a list of
	// `ufbx_node*` instances eg. `ufbx_mesh.instances`.
	vmesh->num_instances = mesh->instances.count;
	vmesh->instance_node_indices = alloc(int32_t, mesh->instances.count);
	for (size_t i = 0; i < mesh->instances.count; i++) {
		vmesh->instance_node_indices[i] = (int32_t)mesh->instances.data[i]->typed_id;
	}

	// Create the vertex buffers
	size_t num_blend_shapes = 0;
	ufbx_blend_channel *blend_channels[MAX_BLEND_SHAPES];
	size_t num_bones = 0;
	ufbx_skin_deformer *skin = NULL;

	if (mesh->skin_deformers.count > 0) {
		vmesh->skinned = true;

		// Having multiple skin deformers attached at once is exceedingly rare so we can just
		// pick the first one without having to worry too much about it.
		skin = mesh->skin_deformers.data[0];

		// NOTE: A proper implementation would split meshes with too many bones to chunks but
		// for simplicity we're going to just pick the first `MAX_BONES` ones.
		for (size_t ci = 0; ci < skin->clusters.count; ci++) {
			ufbx_skin_cluster *cluster = skin->clusters.data[ci];
			if (num_bones < MAX_BONES) {
				vmesh->bone_indices[num_bones] = (int32_t)cluster->bone_node->typed_id;
				vmesh->bone_matrices[num_bones] = ufbx_to_um_mat(cluster->geometry_to_bone);
				num_bones++;
			}
		}
		vmesh->num_bones = num_bones;

		// Pre-calculate the skinned vertex bones/weights for each vertex as they will probably
		// be shared by multiple indices.
		for (size_t vi = 0; vi < mesh->num_vertices; vi++) {
			size_t num_weights = 0;
			float total_weight = 0.0f;
			float weights[4] = { 0.0f };
			uint8_t clusters[4] = { 0 };

			// `ufbx_skin_vertex` contains the offset and number of weights that deform the vertex
			// in a descending weight order so we can pick the first N weights to use and get a
			// reasonable approximation of the skinning.
			ufbx_skin_vertex vertex_weights = skin->vertices.data[vi];
			for (size_t wi = 0; wi < vertex_weights.num_weights; wi++) {
				if (num_weights >= 4) break;
				ufbx_skin_weight weight = skin->weights.data[vertex_weights.weight_begin + wi];

				// Since we only support a fixed amount of bones up to `MAX_BONES` and we take the
				// first N ones we need to ignore weights with too high `cluster_index`.
				if (weight.cluster_index < MAX_BONES) {
					total_weight += (float)weight.weight;
					clusters[num_weights] = (uint8_t)weight.cluster_index;
					weights[num_weights] = (float)weight.weight;
					num_weights++;
				}
			}

			// Normalize and quantize the weights to 8 bits. We need to be a bit careful to make
			// sure the _quantized_ sum is normalized ie. all 8-bit values sum to 255.
			if (total_weight > 0.0f) {
				skin_vertex *skin_vert = &mesh_skin_vertices[vi];
				uint32_t quantized_sum = 0;
				for (size_t i = 0; i < 4; i++) {
					uint8_t quantized_weight = (uint8_t)((float)weights[i] / total_weight * 255.0f);
					quantized_sum += quantized_weight;
					skin_vert->bone_index[i] = clusters[i];
					skin_vert->bone_weight[i] = quantized_weight;
				}
				skin_vert->bone_weight[0] += 255 - quantized_sum;
			}
		}
	}

	// Fetch blend channels from all attached blend deformers.
	for (size_t di = 0; di < mesh->blend_deformers.count; di++) {
		ufbx_blend_deformer *deformer = mesh->blend_deformers.data[di];
		for (size_t ci = 0; ci < deformer->channels.count; ci++) {
			ufbx_blend_channel *chan = deformer->channels.data[ci];
			if (chan->keyframes.count == 0) continue;
			if (num_blend_shapes < MAX_BLEND_SHAPES) {
				blend_channels[num_blend_shapes] = chan;
				vmesh->blend_channel_indices[num_blend_shapes] = (int32_t)chan->typed_id;
				num_blend_shapes++;
			}
		}
	}
	if (num_blend_shapes > 0) {
		vmesh->blend_shape_image = pack_blend_channels_to_image(mesh, blend_channels, num_blend_shapes);
		vmesh->num_blend_shapes = num_blend_shapes;
	}

	// Our shader supports only a single material per draw call so we need to split the mesh
	// into parts by material. `ufbx_mesh_part` contains a handy compact list of faces
	// that use the material which we use here.
	for (size_t pi = 0; pi < mesh->material_parts.count; pi++) {
		ufbx_mesh_part *mesh_part = &mesh->material_parts.data[pi];
		if (mesh_part->num_triangles == 0) continue;

		viewer_mesh_part *part = &parts[num_parts++];
		size_t num_indices = 0;

		// First fetch all vertices into a flat non-indexed buffer, we also need to
		// triangulate the faces
		for (size_t fi = 0; fi < mesh_part->num_faces; fi++) {
			ufbx_face face = mesh->faces.data[mesh_part->face_indices.data[fi]];
			size_t num_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, mesh, face);

			ufbx_vec2 default_uv = { 0 };

			// Iterate through every vertex of every triangle in the triangulated result
			for (size_t vi = 0; vi < num_tris * 3; vi++) {
				uint32_t ix = tri_indices[vi];
				mesh_vertex *vert = &vertices[num_indices];

				ufbx_vec3 pos = ufbx_get_vertex_vec3(&mesh->vertex_position, ix);
				ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, ix);
				ufbx_vec2 uv = mesh->vertex_uv.exists ? ufbx_get_vertex_vec2(&mesh->vertex_uv, ix) : default_uv;

				vert->position = ufbx_to_um_vec3(pos);
				vert->normal = um_normalize3(ufbx_to_um_vec3(normal));
				vert->uv = ufbx_to_um_vec2(uv);
				vert->f_vertex_index = (float)mesh->vertex_indices.data[ix];

				// The skinning vertex stream is pre-calculated above so we just need to
				// copy the right one by the vertex index.
				if (skin) {
					skin_vertices[num_indices] = mesh_skin_vertices[mesh->vertex_indices.data[ix]];
				}

				num_indices++;
			}
		}

		ufbx_vertex_stream streams[2];
		size_t num_streams = 1;

		streams[0].data = vertices;
		streams[0].vertex_count = num_indices;
		streams[0].vertex_size = sizeof(mesh_vertex);

		if (skin) {
			streams[1].data = skin_vertices;
			streams[1].vertex_count = num_indices;
			streams[1].vertex_size = sizeof(skin_vertex);
			num_streams = 2;
		}

		// Optimize the flat vertex buffer into an indexed one. `ufbx_generate_indices()`
		// compacts the vertex buffer and returns the number of used vertices.
		ufbx_error error;
		size_t num_vertices = ufbx_generate_indices(streams, num_streams, indices, num_indices, NULL, &error);
		if (error.type != UFBX_ERROR_NONE) {
			print_error(&error, "Failed to generate index buffer");
			exit(1);
		}

		part->num_indices = num_indices;
		if (mesh_part->index < mesh->materials.count) {
			ufbx_material *material =  mesh->materials.data[mesh_part->index];
			part->material_index = (int32_t)material->typed_id;
		} else {
			part->material_index = -1;
		}

		// Create the GPU buffers from the temporary `vertices` and `indices` arrays
		part->index_buffer = sg_make_buffer(&(sg_buffer_desc){
			.size = num_indices * sizeof(uint32_t),
			.type = SG_BUFFERTYPE_INDEXBUFFER,
			.data = { indices, num_indices * sizeof(uint32_t) },
		});
		part->vertex_buffer = sg_make_buffer(&(sg_buffer_desc){
			.size = num_vertices * sizeof(mesh_vertex),
			.type = SG_BUFFERTYPE_VERTEXBUFFER,
			.data = { vertices, num_vertices * sizeof(mesh_vertex) },
		});

		if (vmesh->skinned) {
			part->skin_buffer = sg_make_buffer(&(sg_buffer_desc){
				.size = num_vertices * sizeof(skin_vertex),
				.type = SG_BUFFERTYPE_VERTEXBUFFER,
				.data = { skin_vertices, num_vertices * sizeof(skin_vertex) },
			});
		}
	}

	// Free the temporary buffers
	free(tri_indices);
	free(vertices);
	free(skin_vertices);
	free(mesh_skin_vertices);
	free(indices);

	// Compute bounds from the vertices
	vmesh->aabb_is_local = mesh->skinned_is_local;
	vmesh->aabb_min = um_dup3(+INFINITY);
	vmesh->aabb_max = um_dup3(-INFINITY);
	for (size_t i = 0; i < mesh->num_vertices; i++) {
		um_vec3 pos = ufbx_to_um_vec3(mesh->skinned_position.values.data[i]);
		vmesh->aabb_min = um_min3(vmesh->aabb_min, pos);
		vmesh->aabb_max = um_max3(vmesh->aabb_max, pos);
	}

	vmesh->parts = parts;
	vmesh->num_parts = num_parts;*/
}

/*void read_blend_channel(viewer_blend_channel *vchan, aiBlendChannel *chan)
{
	vchan->weight = (float)chan->weight;
}*/

/*void read_node_anim(viewer_anim *va, viewer_node_anim *vna, ufbx_anim_stack *stack, ufbx_node *node)
{
	vna->rot = alloc(um_quat, va->num_frames);
	vna->pos = alloc(um_vec3, va->num_frames);
	vna->scale = alloc(um_vec3, va->num_frames);

	bool const_rot = true, const_pos = true, const_scale = true;

	// Sample the node's transform evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_transform transform = ufbx_evaluate_transform(stack->anim, node, time);
		vna->rot[i] = ufbx_to_um_quat(transform.rotation);
		vna->pos[i] = ufbx_to_um_vec3(transform.translation);
		vna->scale[i] = ufbx_to_um_vec3(transform.scale);

		if (i > 0) {
			// Negated quaternions are equivalent, but interpolating between ones of different
			// polarity takes a the longer path, so flip the quaternion if necessary.
			if (um_quat_dot(vna->rot[i], vna->rot[i - 1]) < 0.0f) {
				vna->rot[i] = um_quat_neg(vna->rot[i]);
			}

			// Keep track of which channels are constant for the whole animation as an optimization
			if (!um_quat_equal(vna->rot[i - 1], vna->rot[i])) const_rot = false;
			if (!um_equal3(vna->pos[i - 1], vna->pos[i])) const_pos = false;
			if (!um_equal3(vna->scale[i - 1], vna->scale[i])) const_scale = false;
		}
	}

	if (const_rot) { vna->const_rot = vna->rot[0]; free(vna->rot); vna->rot = NULL; }
	if (const_pos) { vna->const_pos = vna->pos[0]; free(vna->pos); vna->pos = NULL; }
	if (const_scale) { vna->const_scale = vna->scale[0]; free(vna->scale); vna->scale = NULL; }
}*/

/*void read_blend_channel_anim(viewer_anim *va, viewer_blend_channel_anim *vbca, ufbx_anim_stack *stack, ufbx_blend_channel *chan)
{
	vbca->weight = alloc(float, va->num_frames);

	bool const_weight = true;

	// Sample the blend weight evenly for the whole animation stack duration
	for (size_t i = 0; i < va->num_frames; i++) {
		double time = stack->time_begin + (double)i / va->framerate;

		ufbx_real weight = ufbx_evaluate_blend_weight(stack->anim, chan, time);
		vbca->weight[i] = (float)weight;

		// Keep track of which channels are constant for the whole animation as an optimization
		if (i > 0) {
			if (vbca->weight[i - 1] != vbca->weight[i]) const_weight = false;
		}
	}

	if (const_weight) { vbca->const_weight = vbca->weight[0]; free(vbca->weight); vbca->weight = NULL; }
}*/

void read_anim_stack(viewer_anim *va, aiScene *scene)
{
}

void read_scene(viewer_scene *vs, aiScene *scene)
{
}

void update_animation(viewer_scene *vs, viewer_anim *va, float time)
{
}

void update_hierarchy(viewer_scene *vs)
{
	for (size_t i = 0; i < vs->num_nodes; i++) {
		viewer_node *vn = &vs->nodes[i];

		// ufbx stores nodes in order where parent nodes always precede child nodes so we can
		// evaluate the transform hierarchy with a flat loop.
		if (vn->parent_index >= 0) {
			//vn->node_to_world = um_mat_mul(vs->nodes[vn->parent_index].node_to_world, vn->node_to_parent);
		} else {
			vn->node_to_world = vn->node_to_parent;
		}
		//vn->geometry_to_world = um_mat_mul(vn->node_to_world, vn->geometry_to_node);
		//vn->normal_to_world = um_mat_transpose(um_mat_inverse(vn->geometry_to_world));
	}
}

void init_pipelines(viewer *view)
{
	sg_backend backend = sg_query_backend();

	view->shader_mesh_lit_static = sg_make_shader(static_lit_shader_desc(backend));
	view->pipe_mesh_lit_static = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = view->shader_mesh_lit_static,
		.layout = mesh_vertex_layout,
		.index_type = SG_INDEXTYPE_UINT32,
		.face_winding = SG_FACEWINDING_CCW,
		.cull_mode = SG_CULLMODE_BACK,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	view->shader_mesh_lit_skinned = sg_make_shader(skinned_lit_shader_desc(backend));
	view->pipe_mesh_lit_skinned = sg_make_pipeline(&(sg_pipeline_desc){
		.shader = view->shader_mesh_lit_skinned,
		.layout = skinned_mesh_vertex_layout,
		.index_type = SG_INDEXTYPE_UINT32,
		.face_winding = SG_FACEWINDING_CCW,
		.cull_mode = SG_CULLMODE_BACK,
		.depth = {
			.compare = SG_COMPAREFUNC_LESS_EQUAL,
			.write_enabled = true,
		},
	});

	vec4 empty_blend_shape_data = { 0 };
	view->empty_blend_shape_image = sg_make_image(&(sg_image_desc){
		.type = SG_IMAGETYPE_ARRAY,
		.width = 1,
		.height = 1,
		.num_slices = 1,
		.pixel_format = SG_PIXELFORMAT_RGBA32F,
		.data.subimage[0][0] = SG_RANGE(empty_blend_shape_data),
	});
}

void load_scene(viewer_scene *vs, const char *filename)
{
}

bool backend_uses_d3d_perspective(sg_backend backend)
{
	switch (backend) {
	case SG_BACKEND_GLCORE33: return false;
    case SG_BACKEND_GLES2: return false;
    case SG_BACKEND_GLES3: return false;
    case SG_BACKEND_D3D11: return true;
    case SG_BACKEND_METAL_IOS: return true;
    case SG_BACKEND_METAL_MACOS: return true;
    case SG_BACKEND_METAL_SIMULATOR: return true;
    case SG_BACKEND_WGPU: return true;
    case SG_BACKEND_DUMMY: return false;
	default: assert(0 && "Unhandled backend"); return false;
	}
}

void update_camera(viewer *view)
{
}

void draw_mesh(viewer *view, viewer_node *node, viewer_mesh *mesh)
{
}

void draw_scene(viewer *view)
{
	for (size_t mi = 0; mi < view->scene.num_meshes; mi++) {
		viewer_mesh *mesh = &view->scene.meshes[mi];
		for (size_t ni = 0; ni < mesh->num_instances; ni++) {
			viewer_node *node = &view->scene.nodes[mesh->instance_node_indices[ni]];
			draw_mesh(view, node, mesh);
		}
	}
}

viewer g_viewer;
const char *g_filename;

void init(void)
{
    sg_setup(&(sg_desc){
		.context = sapp_sgcontext(),
		.buffer_pool_size = 4096,
		.image_pool_size = 4096,
    });

	stm_setup();

	init_pipelines(&g_viewer);
	load_scene(&g_viewer.scene, g_filename);
}

void onevent(const sapp_event *e)
{
	viewer *view = &g_viewer;

	switch (e->type) {
	case SAPP_EVENTTYPE_MOUSE_DOWN:
		view->mouse_buttons |= 1u << (uint32_t)e->mouse_button;
		break;
	case SAPP_EVENTTYPE_MOUSE_UP:
		view->mouse_buttons &= ~(1u << (uint32_t)e->mouse_button);
		break;
	case SAPP_EVENTTYPE_UNFOCUSED:
		view->mouse_buttons = 0;
		break;
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		if (view->mouse_buttons & 1) {
			float scale = um_min((float)sapp_width(), (float)sapp_height());
			view->camera_yaw -= e->mouse_dx / scale * 180.0f;
			view->camera_pitch -= e->mouse_dy / scale * 180.0f;
			view->camera_pitch = um_clamp(view->camera_pitch, -89.0f, 89.0f);
		}
		break;
	case SAPP_EVENTTYPE_MOUSE_SCROLL:
		view->camera_distance += e->scroll_y * -0.02f;
		view->camera_distance = um_clamp(view->camera_distance, -5.0f, 5.0f);
		break;
	default:
		break;
	}
}

void frame()
{
	static uint64_t last_time;
	float dt = (float)stm_sec(stm_laptime(&last_time));
	dt = std::min(dt, 0.1f);

	viewer_anim *anim = g_viewer.scene.num_animations > 0 ? &g_viewer.scene.animations[0] : NULL;

	if (anim) {
		g_viewer.anim_time += dt;
		if (g_viewer.anim_time >= anim->time_end) {
			g_viewer.anim_time -= anim->time_end - anim->time_begin;
		}
		update_animation(&g_viewer.scene, anim, g_viewer.anim_time);
	}

	update_camera(&g_viewer);
	update_hierarchy(&g_viewer.scene);

	sg_pass_action action = {
		.colors[0] = {
			.action = SG_ACTION_CLEAR,
			.value = { 0.1f, 0.1f, 0.2f },
		},
	};
    sg_begin_default_pass(&action, sapp_width(), sapp_height());

	draw_scene(&g_viewer);

    sg_end_pass();
    sg_commit();
}

void cleanup(void)
{
    sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]) {

	if (argc <= 1) {
		fprintf(stderr, "Usage: viewer file.fbx\n");
		exit(1);
	}

	g_filename = argv[1];

    return (sapp_desc){
        .init_cb = &init,
        .event_cb = &onevent,
        .frame_cb = &frame,
        .cleanup_cb = &cleanup,
        .width = 800,
        .height = 600,
		.sample_count = 4,
        .window_title = "ufbx viewer",
    };
}



} // namespace AssimpViewer
