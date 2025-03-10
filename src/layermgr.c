/*
 * This file is part of OpenPNGStudio. 
 * Copyright (C) 2024-2025 LowByteFox
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <console.h>
#include <context.h>
#include <core/icon_db.h>
#include <ui/line_edit.h>
#include <core/mask.h>
#include <raylib-nuklear.h>
#include <raylib.h>
#include <unuv.h>
#include <limits.h>
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <layer/manager.h>
#include <string.h>

extern struct context ctx;

static void reset_key_mask(uint64_t *mask);
static void draw_props(struct layer_manager *mgr, struct nk_context *ctx);
static nk_bool nk_filter_key(const struct nk_text_edit *box, nk_rune unicode);
static enum un_action after_timeout(un_timer *timer);
static enum un_action after_toggle(un_timer *timer);

void layer_manager_deinit(struct layer_manager *mgr)
{
    /* TODO */
}

struct model_layer *layer_manager_add_layer(struct layer_manager *mgr,
    struct model_layer *layer)
{
    mgr->layers = realloc(mgr->layers, (++mgr->layer_count) * sizeof(struct model_layer*));
    struct model_layer *new_layer = calloc(1, sizeof(struct model_layer));
    *new_layer = *layer;
    mgr->layers[mgr->layer_count - 1] = new_layer;
    return new_layer;
}

void layer_manager_draw_ui(struct layer_manager *mgr, struct nk_context *ctx)
{
    struct nk_rect bounds = nk_window_get_content_region(ctx);
    int res = 0;
    if (mgr->selected_index != -1) {
        bounds.h /= 2;
        bounds.h -= 10;
        nk_layout_row_dynamic(ctx, bounds.h, 1);
        res = nk_group_begin(ctx, "Layer", 0);
    }

    for (int i = 0; i < mgr->layer_count; i++) {
        struct model_layer *layer = mgr->layers[i];
        struct nk_rect bounds = nk_layout_widget_bounds(ctx);
        bounds.h *= 2;

        nk_layout_row_template_begin(ctx, 30);
        nk_layout_row_template_push_static(ctx, 30);
        nk_layout_row_template_push_dynamic(ctx);
        nk_layout_row_template_end(ctx);

        if (nk_button_image(ctx, get_icon(SELECT_ICON)))
            mgr->selected_index = i;

        line_edit_draw(&layer->name, ctx);

        /* UI controls */
        nk_layout_row_template_begin(ctx, 30);
        nk_layout_row_template_push_static(ctx, 30);
        nk_layout_row_template_push_static(ctx, 30);
        nk_layout_row_template_push_dynamic(ctx);
        nk_layout_row_template_push_static(ctx, 30);
        nk_layout_row_template_end(ctx);

        if (i == 0)
            nk_widget_disable_begin(ctx);

        if (nk_button_image(ctx, get_icon(UP_ICON))) {
            struct model_layer *prev = mgr->layers[i - 1];
            mgr->layers[i - 1] = layer;
            mgr->layers[i] = prev;
            mgr->selected_index = -1;
        }

        if (i == 0)
            nk_widget_disable_end(ctx);

        if (i == mgr->layer_count - 1)
            nk_widget_disable_begin(ctx);

        if (nk_button_image(ctx, get_icon(DOWN_ICON))) {
            struct model_layer *next = mgr->layers[i + 1];
            mgr->layers[i + 1] = layer;
            mgr->layers[i] = next;
            mgr->selected_index = -1;
        }

        nk_spacer(ctx);

        if (i == mgr->layer_count - 1)
            nk_widget_disable_end(ctx);

        if (nk_button_image(ctx, get_icon(TRASH_ICON))) {
            layer->delete = true;
            mgr->selected_index = -1;
        }
    }

    if (res)
        nk_group_end(ctx);

    if (mgr->selected_index != -1) {
        nk_layout_row_dynamic(ctx, 2, 1);
        nk_rule_horizontal(ctx, ctx->style.window.border_color, false);
        nk_layout_row_dynamic(ctx, bounds.h, 1);
        struct model_layer *layer = mgr->layers[mgr->selected_index];
        uint64_t old_mask = layer->mask;
        if (nk_group_begin(ctx, "Layer Property Editor", 0))
            draw_props(mgr, ctx);

        if (layer->mask != old_mask)
            layer->is_togged = false;
    }

    for (int i = 0; i < mgr->layer_count;) {
        struct model_layer *layer = mgr->layers[i];

        if (layer->delete) {
            if (i + 1 < mgr->layer_count)
                memcpy(mgr->layers + i, mgr->layers + i + 1,
                    sizeof(struct model_layer*) * (mgr->layer_count - i - 1));

            mgr->layer_count--;
            if (layer->alive || layer->frames_count > 0)
                continue; /* it will get cleaned up in the respective fn */

            cleanup_layer(layer);
            continue;
        }

        /* if layer didn't get deleted */
        i++;
    }
}

void layer_manager_draw_layers(struct layer_manager *mgr)
{
    float width = GetScreenWidth();
    float height = GetScreenHeight();

    for (int i = 0; i < mgr->layer_count; i++) {
        struct model_layer *layer = mgr->layers[i];
        Image *img = &layer->img;
        Texture2D texture = layer->texture;

        if (layer->frames_count > 0) {
            if (layer->previous_frame != layer->current_frame) {
                size_t offset = img->width * img->height * 4 *
                    layer->current_frame;

                UpdateTexture(texture, ((unsigned char*) img->data) + offset);
                layer->previous_frame = layer->current_frame;
            }
        }

        bool mask_test = test_mask(mgr->mask, layer->mask);

        if ((layer->alive || layer->is_togged) || mask_test) {
            DrawTexturePro(texture, (Rectangle) {
                .x = 0,
                .y = 0,
                .width = texture.width,
                .height = texture.height,
            }, (Rectangle) {
                .x = width / 2.0f + layer->position_offset.x,
                .y = height / 2.0f + (-layer->position_offset.y),
                .width = texture.width,
                .height = texture.height,
            }, (Vector2) {
                .x = texture.width / 2.0f,
                .y = texture.height / 2.0f,
            }, layer->rotation - 180, WHITE);

            if (!layer->has_toggle) {
                if (!layer->alive && layer->ttl > 0) {
                    /* spawn live timeout */
                    layer->alive = true;
                    un_timer *timer = un_timer_new(ctx.loop);
                    un_timer_set_data(timer, layer);
                    un_timer_start(timer, layer->ttl, 0, after_timeout);
                }
            } else {
                if (!layer->toggle_timer_ticking && mask_test) {
                    layer->is_togged = !layer->is_togged;
                    layer->toggle_timer_ticking = true;
                    un_timer *timer = un_timer_new(ctx.loop);
                    un_timer_set_data(timer, layer);
                    un_timer_start(timer, 250, 0, after_toggle);
                }
            }
        }
    }
}

char *layer_tomlify(struct model_layer *layer)
{
    const char *fmt = "[layer]\n"
                      "offset.x = %f\n"
                      "offset.y = %f\n"
                      "rotation = %f\n"
                      "mask = %ld\n"
                      "ttl = %d\n"
                      "has_toggle = %s\n";

    int len = snprintf(NULL, 0, fmt, layer->position_offset.x,
        layer->position_offset.y, layer->rotation, layer->mask, layer->ttl,
        layer->has_toggle == true ? "true" : "false");

    char *buffer = calloc(1, len + 1);
    snprintf(buffer, len + 1, fmt, layer->position_offset.x,
        layer->position_offset.y, layer->rotation, layer->mask, layer->ttl,
        layer->has_toggle == true ? "true" : "false");

    if (layer->frames_count > 0) {
        fmt = "[animation]\n"
              "frame_count = %d\n"
              "delays = [ ";
        int add_length = snprintf(NULL, 0, fmt, layer->frames_count);
        buffer = realloc(buffer, len + add_length + 1);

        snprintf(buffer + len, add_length + 1, fmt, layer->frames_count);
        len += add_length;

        fmt = "%d, ";
        int i;
        for (i = 0; i < layer->frames_count - 1; i++) {
            add_length = snprintf(NULL, 0, fmt, layer->delays[i]);
            buffer = realloc(buffer, len + add_length + 1);

            snprintf(buffer + len, add_length + 1, fmt, layer->delays[i]);
            len += add_length;
        }

        fmt = "%d ]\n";
        add_length = snprintf(NULL, 0, fmt, layer->delays[i]);
        buffer = realloc(buffer, len + add_length + 1);

        snprintf(buffer + len, add_length + 1, fmt, layer->delays[i]);
        len += add_length;
    }

    return buffer;
}

void cleanup_layer(struct model_layer *layer)
{
    UnloadImage(layer->img);
    UnloadTexture(layer->texture);
    free(layer);
}

static void draw_props(struct layer_manager *mgr, struct nk_context *ctx)
{
    struct model_layer *layer = mgr->layers[mgr->selected_index];
    bool holding_shift = nk_input_is_key_down(&ctx->input, NK_KEY_SHIFT);

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Position:", NK_TEXT_LEFT);
    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
    nk_layout_row_push(ctx, 0.5f);
    if (!holding_shift) {
        nk_property_float(ctx, "X", -FLT_MAX, &layer->position_offset.x, FLT_MAX, 0.1f, 0.1f);
        nk_layout_row_push(ctx, 0.49f);
        nk_property_float(ctx, "Y", -FLT_MAX, &layer->position_offset.y, FLT_MAX, 0.1f, 0.1f);
    } else {
        layer->position_offset.x = roundf(layer->position_offset.x);
        layer->position_offset.y = roundf(layer->position_offset.y);
        nk_property_float(ctx, "X", -FLT_MAX, &layer->position_offset.x, FLT_MAX, 1, 1);
        nk_layout_row_push(ctx, 0.49f);
        nk_property_float(ctx, "Y", -FLT_MAX, &layer->position_offset.y, FLT_MAX, 1, 1);
    }
    nk_layout_row_end(ctx);

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Rotation:", NK_TEXT_LEFT);
    nk_layout_row_static(ctx, 40, 40, 1);

    nk_knob_float(ctx, 0, &layer->rotation, 360, 0.1f, NK_DOWN, 0.0f);

    if (holding_shift)
        layer->rotation = roundf(layer->rotation / 15.0f) * 15.0f;

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_checkbox_label(ctx, "Enable toggle mode", &layer->has_toggle);

    if (layer->has_toggle)
        nk_widget_disable_begin(ctx);

    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
    nk_layout_row_push(ctx, 0.5f);
    nk_label(ctx, "Time to live:", NK_TEXT_LEFT);
    nk_layout_row_push(ctx, 0.49f);
    nk_property_int(ctx, "timeout (ms)", 0, &layer->ttl, INT_MAX, 1, 0.1f);

    if (layer->has_toggle)
        nk_widget_disable_end(ctx);

    nk_layout_row_dynamic(ctx, 2, 1);
    nk_rule_horizontal(ctx, ctx->style.window.border_color, false);

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Mask:", NK_TEXT_LEFT);
    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
    
    nk_layout_row_push(ctx, 0.33f);
    nk_checkbox_flags_label(ctx, "Normal", (unsigned int*) &layer->mask, QUIET);
    nk_layout_row_push(ctx, 0.33f);
    nk_checkbox_flags_label(ctx, "Talking", (unsigned int*) &layer->mask, TALK);
    nk_layout_row_push(ctx, 0.329f);
    nk_checkbox_flags_label(ctx, "Pause", (unsigned int*) &layer->mask, PAUSE);
    nk_layout_row_end(ctx);

    nk_layout_row_dynamic(ctx, 30, 1);
    nk_label(ctx, "Modifier keys:", NK_TEXT_LEFT);
    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 4);

    nk_layout_row_push(ctx, 0.25f);
    nk_checkbox_flags_label(ctx, "Shift", (unsigned int*) &layer->mask, SHIFT);
    nk_layout_row_push(ctx, 0.25f);
    nk_checkbox_flags_label(ctx, "Ctrl", (unsigned int*) &layer->mask, CTRL);
    nk_layout_row_push(ctx, 0.25f);
    nk_checkbox_flags_label(ctx, "Super", (unsigned int*) &layer->mask, SUPER);
    nk_layout_row_push(ctx, 0.249f);
    nk_checkbox_flags_label(ctx, "Alt", (unsigned int*) &layer->mask, META);
    nk_layout_row_end(ctx);

    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 2);
    nk_layout_row_push(ctx, 0.5f);
    nk_label(ctx, "Key press:", NK_TEXT_LEFT);
    nk_layout_row_push(ctx, 0.49f);
    nk_edit_string(ctx, NK_EDIT_SIMPLE, layer->input_key, &layer->input_len, 2,
        nk_filter_key);

    if (layer->input_len > 0) {
        layer->input_key[0] = toupper(layer->input_key[0]);
        layer->mask |= 1ULL << (layer->input_key[0] - 'A' + KEY_START);
    } else
        reset_key_mask(&layer->mask);

    nk_group_end(ctx);
}

static enum un_action after_timeout(un_timer *timer)
{
    struct model_layer *layer = un_timer_get_data(timer);
    layer->alive = false;

    if (layer->delete) {
        if (layer->frames_count == 0)
            cleanup_layer(layer); /* not a GIF */
    }

    return DISARM;
}

static enum un_action after_toggle(un_timer *timer)
{
    struct model_layer *layer = un_timer_get_data(timer);
    layer->toggle_timer_ticking = false;

    return DISARM;
}

static void reset_key_mask(uint64_t *mask)
{
    uint64_t new_mask = 0;
    for (int i = 0; i < 7; i++)
        new_mask |= 1ULL << i;

    *mask &= new_mask;
}

static nk_bool nk_filter_key(const struct nk_text_edit *box, nk_rune unicode)
{
    if ((unicode >= 'a' && unicode <= 'z') || (unicode >= 'A' && unicode <= 'Z'))
        return nk_true;

    return nk_false;
}
