#include <stdlib.h>

#include "components/rigidbody.h"
#include "scene.h"


RigidBodyComponent* RigidBodyComponent_add(Entity entity, RigidBodyParameters params) {
    RigidBodyComponent* rigid_body = malloc(sizeof(RigidBodyComponent));

    rigid_body->velocity = zeros3();
    rigid_body->acceleration = zeros3();
    rigid_body->angular_velocity = zeros3();
    rigid_body->angular_acceleration = zeros3();
    rigid_body->inv_mass = params.mass ? 1.0f / params.mass : 1.0f;
    rigid_body->friction = params.friction;
    rigid_body->bounce = params.bounce;
    rigid_body->gravity_scale = 1.0f;
    rigid_body->can_sleep = !params.dont_sleep;
    rigid_body->asleep = false;
    rigid_body->on_ground = false;
    rigid_body->angular_damping = 0.98f;
    rigid_body->linear_damping = 0.999f;
    rigid_body->inv_inertia = identity3();
    rigid_body->max_speed = 10.0f;
    rigid_body->max_angular_speed = 2.0f;
    rigid_body->axis_lock.x = params.axis_lock.x;
    rigid_body->axis_lock.y = params.axis_lock.y;
    rigid_body->axis_lock.z = params.axis_lock.z;
    rigid_body->axis_lock.rotation = params.axis_lock.rotation;
    rigid_body->axis_lock.rotation_axis = vec3_up();
    if (non_zero3(params.axis_lock.rotation_axis)) {
        rigid_body->axis_lock.rotation_axis = normalized3(params.axis_lock.rotation_axis);
    }
    rigid_body->springs = ArrayList_create(sizeof(Spring));
    strcpy(rigid_body->move_sound, params.move_sound);

    scene->components->rigid_body[entity] = rigid_body;

    return rigid_body;
}


void RigidBodyComponent_remove(Entity entity) {
    RigidBodyComponent* ridig_body = scene->components->rigid_body[entity];
    if (ridig_body) {
        ArrayList_destroy(ridig_body->springs);
        free(ridig_body);
        scene->components->rigid_body[entity] = NULL;
    }
}


void add_spring(Entity entity, Spring spring) {
    RigidBodyComponent* rb = get_component(entity, COMPONENT_RIGIDBODY);
    if (!rb) return;

    ArrayList_add(rb->springs, &spring);
}
