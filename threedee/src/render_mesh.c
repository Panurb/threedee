#include "render_mesh.h"
#include "render.h"
#include "app.h"


MeshData create_mesh_triangle() {
	MeshData mesh_data = {
		.name = "triangle",
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh_data.num_vertices = 3;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector3) * mesh_data.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector3) * mesh_data.num_vertices,
        }
    );

    Vector3* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vector3) { 0.0f, 0.0f, 0.0f };
	transfer_data[1] = (Vector3) { 1.0f, 0.0f, 0.0f };
	transfer_data[2] = (Vector3) { 0.0f, 1.0f, 0.0f };

    SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = mesh_data.vertex_buffer,
            .offset = 0,
            .size = sizeof(Vector3) * mesh_data.num_vertices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}


MeshData create_mesh_triangle_2d() {
	MeshData mesh_data = {
		.name = "triangle_2d",
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh_data.num_vertices = 3;
    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector2) * mesh_data.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector2) * mesh_data.num_vertices,
        }
    );

    Vector2* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

    transfer_data[0] = (Vector2) { 0.0f, 0.0f };
	transfer_data[1] = (Vector2) { 1.0f, 0.0f };
	transfer_data[2] = (Vector2) { 0.0f, 1.0f };

    SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = mesh_data.vertex_buffer,
            .offset = 0,
            .size = sizeof(Vector2) * mesh_data.num_vertices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}


MeshData create_mesh_quad() {
	MeshData mesh_data = {
		.name = "quad",
		.num_vertices = 4,
		.num_indices = 6,
	};

	mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices,
		}
	);

	mesh_data.index_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * mesh_data.num_indices,
		}
	);

	SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
		}
	);

	PositionTextureVertex* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);
	transfer_data[0] = (PositionTextureVertex) {
		.position = { -0.5f, -0.5f, 0.0f },
		.uv = { 0.0f, 1.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[1] = (PositionTextureVertex) {
		.position = { 0.5f, -0.5f, 0.0f },
		.uv = { 1.0f, 1.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[2] = (PositionTextureVertex) {
		.position = { 0.5f, 0.5f, 0.0f },
		.uv = { 1.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};
	transfer_data[3] = (PositionTextureVertex) {
		.position = { -0.5f, 0.5f, 0.0f },
		.uv = { 0.0f, 0.0f },
		.normal = { 0.0f, 0.0f, 1.0f },
		.tangent = { 1.0f, 0.0f, 0.0f },
	};

	Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
	index_data[0] = 0;
	index_data[1] = 1;
	index_data[2] = 2;
	index_data[3] = 2;
	index_data[4] = 3;
	index_data[5] = 0;

	SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

	SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = 0
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data.vertex_buffer,
			.offset = 0,
			.size = sizeof(PositionTextureVertex) * mesh_data.num_vertices
		},
		false
	);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex) * mesh_data.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh_data.num_indices
		},
		false
	);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_command_buffer);
	SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}


MeshData create_mesh_line() {
	MeshData mesh_data = {
		.name = "line",
		.num_vertices = 4,
		.num_indices = 0,
	};

	return mesh_data;
}


Vector2 get_text_center(TTF_GPUAtlasDrawSequence data) {
	float min_x = INFINITY;
	float min_y = INFINITY;
	float max_x = -INFINITY;
	float max_y = -INFINITY;

	for (int i = 0; i < data.num_vertices; ++i) {
		SDL_FPoint xy = data.xy[i];
		min_x = fminf(min_x, xy.x);
		min_y = fminf(min_y, xy.y);
		max_x = fmaxf(max_x, xy.x);
		max_y = fmaxf(max_y, xy.y);
	}

	return (Vector2) {
		.x = (min_x + max_x) * 0.5f,
		.y = (min_y + max_y) * 0.5f
	};
}


MeshData create_mesh_text(TTF_GPUAtlasDrawSequence data) {
	MeshData mesh_data = {
		.name = "text",
		.num_vertices = data.num_vertices,
		.num_indices = data.num_indices,
		.texture = data.atlas_texture,
	};

    mesh_data.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices,
        }
    );

	mesh_data.index_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * mesh_data.num_indices,
		}
	);

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        	.size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices + sizeof(Uint16) * mesh_data.num_indices,
        }
    );

    PositionTextureVertex2D* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

	Vector2 text_center = get_text_center(data);
	float w = text_center.x;
	float h = text_center.y;

    for (int i = 0; i < mesh_data.num_vertices; ++i) {
    	SDL_FPoint xy = data.xy[i];
    	SDL_FPoint uv = data.uv[i];
		transfer_data[i] = (PositionTextureVertex2D) { xy.x - w, xy.y - h, uv.x, uv.y };
	}

	Uint16* index_data = (Uint16*) &transfer_data[mesh_data.num_vertices];
	for (int i = 0; i < mesh_data.num_indices; ++i) {
		index_data[i] = data.indices[i];
	}

    SDL_UnmapGPUTransferBuffer(app.gpu_device, transfer_buffer);

    SDL_GPUCommandBuffer* upload_command_buffer = SDL_AcquireGPUCommandBuffer(app.gpu_device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_command_buffer);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = mesh_data.vertex_buffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices
        },
        false
    );

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex2D) * mesh_data.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh_data.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh_data.num_indices
		},
		false
	);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh_data;
}
