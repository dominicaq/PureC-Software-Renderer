#ifndef RVC
#include <SDL.h>
#include <time.h>
#endif

#include "mango.h"
#include "render/drawing.h"
#include "render/framedata.h"

#ifndef RVC
uint32_t controller_mask;
uint32_t get_controller() { return controller_mask; }
#endif

void mango_play_anim(Mango *mango, int object_index, AnimStack *stack) {
    for (int i = 0; i < mango->running_anims.len; ++i) {
        Anim *anim = mango->running_anims.arr + i;
        if (anim->time_progress >= anim->stack.time_end) {
            *anim = (Anim){0, object_index, *stack};
        }
    }
}

void mango_update_anim(Mango *mango, Anim *anim, Real dt) {
    // if (anim->time_progress >= anim->stack.time_end) {
    //     return;
    // }
    // anim->time_progress += dt;
    // if (anim->time_progress < anim->stack.time_begin) {
    //     return;
    // }
    // for (int i = 0; i < anim->stack.layers.len; ++i) {
    //     AnimLayer *layer = &anim->stack.layers.arr[i];
    //     for (int j = 0; j < layer->anim_props.len; ++j) {
    //         AnimProp *prop = &layer->anim_props.arr[i];
    //         GameObject *obj = &mango->scene->objects[prop->node_index];
    //         prop_update(prop, obj, anim->time_progress);
    //     }
    // }
}

void mango_update(void *arg) {
    Mango *mango = (Mango *)arg;
    Real current_time = clock();
    Real dt = current_time - mango->last_time;
    mango->last_time = current_time;
    mango->user_update(dt);
    frame_reset(mango->frame);

    Scene *current_scene = mango->scene;
    Camera *current_camera = current_scene->camera;
    mango->ubo.u_cam_pos = current_camera->game_object.position;

    for (int i = 0; i < mango->running_anims.len; ++i) {
        mango_update_anim(mango, &mango->running_anims.arr[i], dt);
    }

    // Update scene transforms
    scene_update_matrices(current_scene);
    camera_update_matrix(current_camera);
    // idealy later we here we will organize scene into bvh and then add
    // and remove lights from a light stack ad we render objects

    // Update MVP Matrix: projection * view * model (multiplication order)
    Mat4 projection_matrix = perspective(current_camera);
    Mat4 view_matrix = mat4_inverse(current_camera->game_object.world_matrix);
    for (int i = 0; i < current_scene->object_count; ++i) {
        if (current_scene->attributes[i].type != ATTR_MESH) {
            continue;
        }
        Mesh *target_mesh = &current_scene->attributes[i].mesh;
        GameObject *target_object = current_scene->objects + i;

        // Update matricies
        Mat4 model_matrix = target_object->world_matrix;
        Mat4 model_view_matrix = mat4_mul(view_matrix, model_matrix);
        Mat4 mvp = mat4_mul(projection_matrix, model_view_matrix);
        Mat4 vp = mat4_mul(projection_matrix, view_matrix);

        // Update UBO
        mango->ubo.u_mvp = mvp;
        mango->ubo.u_vp_inv = mat4_inverse(vp);
        mango->ubo.u_model_view = model_view_matrix;
        mango->ubo.u_color = target_mesh->color;
        mango->ubo.u_mat = target_mesh->material;

        draw_mesh(mango->frame, target_mesh, &mango->ubo);
    }

    frame_update(mango->frame);
}

Mango *mango_alloc(Scene *scene, const char *title, int width, int height) {
    Mango *mango = (Mango *)malloc(sizeof(Mango));
    if (mango == NULL) {
        printf("mango_alloc mango malloc failed\n");
        return NULL;
    }
    printf("allocated mango\n");
    mango->scene = scene;
    mango->frame = frame_alloc(title, width, height);
    if (mango->frame == NULL) {
        printf("mango_alloc frame malloc failed\n");
        return NULL;
    }
    printf("allocated frame\n");

    mango->ubo.options = scene->options;
    mango->running_anims.len = 64;
    mango->running_anims.arr =
        (Anim *)malloc(mango->running_anims.len * sizeof(Anim));
    if (mango->running_anims.arr == NULL) {
        printf("mango_alloc anims malloc failed\n");
        return NULL;
    }
    printf("allocated animations\n");
    for (int i = 0; i < mango->running_anims.len; ++i) {
        mango->running_anims.arr[i].time_progress = 1;
        mango->running_anims.arr[i].stack.time_end = 0;
    }

    int num_lights = 0;
    for (int i = 0; i < scene->object_count; ++i) {
        if (scene->attributes[i].type == ATTR_LIGHT) {
            mango->ubo.lights[num_lights] = &scene->attributes[i].light;
            mango->ubo.light_objects[num_lights] = &scene->objects[i];
            ++num_lights;
        }
    }
    mango->ubo.num_lights = num_lights;
    return mango;
}

void mango_run(Mango *mango) {
    printf("running mango\n");
#ifdef RVC
    mango->last_time = clock();
    printf("running mango %d", mango->last_time);
    while (1) {
        mango_update(mango);
    }
#else
    mango->last_time = clock();
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    controller_mask |= INPUT_DIRECTION_UP;
                    break;
                case SDLK_a:
                    controller_mask |= INPUT_DIRECTION_LEFT;
                    break;
                case SDLK_s:
                    controller_mask |= INPUT_DIRECTION_DOWN;
                    break;
                case SDLK_d:
                    controller_mask |= INPUT_DIRECTION_RIGHT;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.sym) {
                case SDLK_w:
                    controller_mask &= ~INPUT_DIRECTION_UP;
                    break;
                case SDLK_a:
                    controller_mask &= ~INPUT_DIRECTION_LEFT;
                    break;
                case SDLK_s:
                    controller_mask &= ~INPUT_DIRECTION_DOWN;
                    break;
                case SDLK_d:
                    controller_mask &= ~INPUT_DIRECTION_RIGHT;
                    break;
                }
                break;
            default:
                break;
            }
        }
        mango_update(mango);
    }
#endif
}

void mango_free(Mango *mango) { frame_free(mango->frame); }
