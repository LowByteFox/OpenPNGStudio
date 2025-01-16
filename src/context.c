#ifdef _WIN32
#include <raylib_win32.h>
#endif

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
