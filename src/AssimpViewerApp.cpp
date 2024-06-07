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
