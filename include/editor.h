#ifndef _EDITOR_H_
#define _EDITOR_H_

#include <layermgr.h>
#include <raylib-nuklear.h>

enum editor_tab_id {
    OVERVIEW,
    LAYERS,
    SCRIPTS
};

struct editor {
    /* TABS */
    struct layer_manager layer_manager;

    /* STATE */
    enum editor_tab_id current_tab;
    struct nk_rect geometry;
    bool hide;
};

void editor_draw(struct editor *editor, struct nk_context *ctx, bool *ui_focused);

#endif