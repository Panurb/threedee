#include "render_mesh.h"

#include <stdio.h>

#include "render.h"
#include "app.h"


Mesh create_mesh_triangle() {
	Mesh mesh = {
		.name = "triangle",
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh.num_vertices = 3;
    mesh.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector3) * mesh.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector3) * mesh.num_vertices,
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
            .buffer = mesh.vertex_buffer,
            .offset = 0,
            .size = sizeof(Vector3) * mesh.num_vertices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh;
}


Mesh create_mesh_triangle_2d() {
	Mesh mesh = {
		.name = "triangle_2d",
		.num_indices = 0,
		.index_buffer = NULL,
	};

	mesh.num_vertices = 3;
    mesh.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(Vector2) * mesh.num_vertices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(Vector2) * mesh.num_vertices,
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
            .buffer = mesh.vertex_buffer,
            .offset = 0,
            .size = sizeof(Vector2) * mesh.num_vertices
        },
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh;
}


Mesh create_mesh_quad() {
	Mesh mesh = {
		.name = "quad",
		.num_vertices = 4,
		.num_indices = 6,
	};

	mesh.vertex_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
			.size = sizeof(PositionTextureVertex) * mesh.num_vertices,
		}
	);

	mesh.index_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * mesh.num_indices,
		}
	);

	SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
		app.gpu_device,
		&(SDL_GPUTransferBufferCreateInfo){
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = sizeof(PositionTextureVertex) * mesh.num_vertices + sizeof(Uint16) * mesh.num_indices,
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

	Uint16* index_data = (Uint16*) &transfer_data[mesh.num_vertices];
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
			.buffer = mesh.vertex_buffer,
			.offset = 0,
			.size = sizeof(PositionTextureVertex) * mesh.num_vertices
		},
		false
	);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex) * mesh.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh.num_indices
		},
		false
	);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_command_buffer);
	SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh;
}


Mesh create_mesh_line() {
	Mesh mesh = {
		.name = "line",
		.num_vertices = 4,
		.num_indices = 0,
	};

	return mesh;
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


Mesh create_mesh_text(TTF_GPUAtlasDrawSequence data) {
	Mesh mesh = {
		.name = "text",
		.num_vertices = data.num_vertices,
		.num_indices = data.num_indices,
		.texture = data.atlas_texture,
	};

    mesh.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex2D) * mesh.num_vertices,
        }
    );

	mesh.index_buffer = SDL_CreateGPUBuffer(
		app.gpu_device,
		&(SDL_GPUBufferCreateInfo){
			.usage = SDL_GPU_BUFFERUSAGE_INDEX,
			.size = sizeof(Uint16) * mesh.num_indices,
		}
	);

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        	.size = sizeof(PositionTextureVertex2D) * mesh.num_vertices + sizeof(Uint16) * mesh.num_indices,
        }
    );

    PositionTextureVertex2D* transfer_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);

	Vector2 text_center = get_text_center(data);
	float w = text_center.x;
	float h = text_center.y;

    for (int i = 0; i < mesh.num_vertices; ++i) {
    	SDL_FPoint xy = data.xy[i];
    	SDL_FPoint uv = data.uv[i];
		transfer_data[i] = (PositionTextureVertex2D) { xy.x - w, xy.y - h, uv.x, uv.y };
	}

	Uint16* index_data = (Uint16*) &transfer_data[mesh.num_vertices];
	for (int i = 0; i < mesh.num_indices; ++i) {
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
            .buffer = mesh.vertex_buffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex2D) * mesh.num_vertices
        },
        false
    );

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex2D) * mesh.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh.num_indices
		},
		false
	);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buffer);
    SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh;
}


Mesh create_mesh_from_face_group(ArrayList* group) {
    Mesh mesh = {
        .name = "face_group",
        .num_vertices = group->size * 4,
        .num_indices = group->size * 6,
    };
	LOG_INFO("Creating mesh from face group with %d faces (%d vertices, %d indices)", group->size, mesh.num_vertices, mesh.num_indices);

    mesh.vertex_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex) * mesh.num_vertices,
        }
    );

    mesh.index_buffer = SDL_CreateGPUBuffer(
        app.gpu_device,
        &(SDL_GPUBufferCreateInfo){
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * mesh.num_indices,
        }
    );

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        app.gpu_device,
        &(SDL_GPUTransferBufferCreateInfo){
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(PositionTextureVertex) * mesh.num_vertices + sizeof(Uint16) * mesh.num_indices,
        }
    );

    PositionTextureVertex* vertex_data = SDL_MapGPUTransferBuffer(app.gpu_device, transfer_buffer, false);
    Uint16* index_data = (Uint16*) &vertex_data[mesh.num_vertices];

    for (int i = 0; i < group->size; i++) {
        CubeFace* face = ArrayList_get(group, i);

        for (int v = 0; v < 4; v++) {
            Vector3 corner = face->corners[v];
            Vector2 uv = face->uvs[v];

            vertex_data[i * 4 + v] = (PositionTextureVertex) {
                .position = corner,
                .uv = uv,
                .normal = face->normal,
                .tangent = face->tangent,
            };
        }

        index_data[i * 6 + 0] = i * 4 + 0;
        index_data[i * 6 + 1] = i * 4 + 1;
        index_data[i * 6 + 2] = i * 4 + 2;
        index_data[i * 6 + 3] = i * 4 + 2;
        index_data[i * 6 + 4] = i * 4 + 3;
        index_data[i * 6 + 5] = i * 4 + 0;
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
			.buffer = mesh.vertex_buffer,
			.offset = 0,
			.size = sizeof(PositionTextureVertex) * mesh.num_vertices
		},
		false
	);

	SDL_UploadToGPUBuffer(
		copy_pass,
		&(SDL_GPUTransferBufferLocation) {
			.transfer_buffer = transfer_buffer,
			.offset = sizeof(PositionTextureVertex) * mesh.num_vertices
		},
		&(SDL_GPUBufferRegion) {
			.buffer = mesh.index_buffer,
			.offset = 0,
			.size = sizeof(Uint16) * mesh.num_indices
		},
		false
	);
	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(upload_command_buffer);
	SDL_ReleaseGPUTransferBuffer(app.gpu_device, transfer_buffer);

	return mesh;
}
