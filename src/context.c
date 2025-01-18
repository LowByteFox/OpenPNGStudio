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

#include "raylib.h"
#ifdef _WIN32
#include <raylib_win32.h>
#endif

#include "console.h"
#include "ui/window.h"

#include "uv.h"
#include <context.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/mman.h>
#else
#include <mman.h>
#endif

void context_load_image(struct context *ctx, const char *name,
    int fd, size_t size, uv_work_cb work, uv_after_work_cb after)
{
    struct image_load_req *req = NULL;
    if (ctx->image_work_queue == NULL)
        req = calloc(1, sizeof(struct image_load_req));
    else {
        req = ctx->image_work_queue;

        while (req->next)
            req = req->next;

        req->next = calloc(1, sizeof(struct image_load_req));
        req = req->next;
    }

    /* prepare request */
    req->buffer = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (req->buffer == MAP_FAILED) {
        perror("mmap");
        abort();
    }
    req->size = size;
    req->name = strdup(name);
    req->ext = strrchr(req->name, '.');
    req->fd = fd;

    /* deploy */
    if (ctx->image_work_queue == NULL)
        ctx->image_work_queue = req;

    if (strcmp(req->ext, ".gif") == 0)
        req->gif_buffer = malloc(size);

    req->req.data = req;
    uv_queue_work((uv_loop_t*) ctx->loop, &req->req, work, after);
}

void context_about(struct context *context, struct nk_context *ctx)
{
    bool is_on = false;

    if (context->about_win.ctx == NULL) {
        int width = GetScreenWidth();
        int height = GetScreenHeight();
        float w = 384.0f;
        float h = 384.0f;
        float x = width / 2.0f - w / 2.0f;
        float y = height / 2.0f - h / 2.0f;

        context->about_win.geometry = nk_rect(x, y, w, h);
        window_init(&context->about_win, ctx, "About OpenPNGStudio");
    }

    if (window_begin(&context->about_win, NK_WINDOW_TITLE | NK_WINDOW_MOVABLE |
        NK_WINDOW_SCALABLE | NK_WINDOW_BORDER | NK_WINDOW_CLOSABLE)) {
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, "OpenPNGStudio", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(ctx, 14, 1);
        nk_label(ctx, "Create & stream PNGTuber", NK_TEXT_CENTERED);
        nk_label(ctx, "models with ease", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_spacer(ctx);
        nk_label(ctx, "[Icon]", NK_TEXT_CENTERED);
        nk_spacer(ctx);
        nk_label(ctx, "Copyright (C) 2024-2025 LowByteFox", NK_TEXT_CENTERED);
        nk_layout_row_dynamic(ctx, 14, 1);
        nk_label(ctx, "License:", NK_TEXT_CENTERED);
        nk_style_push_color(ctx, &ctx->style.text.color,
            nk_rgb(0, 0x63, 0xE1));

        struct nk_rect bounds = nk_widget_bounds(ctx);
        if (nk_input_is_mouse_hovering_rect(&ctx->input, bounds))
            is_on = true;

        if (nk_input_mouse_clicked(&ctx->input, NK_BUTTON_LEFT, bounds)) {
            SetClipboardText("https://www.gnu.org/licenses/gpl-3.0.txt");
        }

        nk_label(ctx, "GNU General Public License v3.0 or later", NK_TEXT_CENTERED);
        nk_style_pop_color(ctx);

        if (is_on) {
            nk_spacer(ctx);
            nk_style_push_color(ctx, &ctx->style.text.color,
                nk_rgb(0xFF, 0xDD, 0x33));
            nk_label(ctx, "By clicking the text, the content",
                NK_TEXT_CENTERED);
            nk_label(ctx, "of your clipboard will be overriden",
                NK_TEXT_CENTERED);
            nk_label(ctx, "to the URL of the license!", NK_TEXT_CENTERED);
            nk_style_pop_color(ctx);
        }
    }

    if (context->about_win.state != HIDE)
        window_end(&context->about_win);
}
