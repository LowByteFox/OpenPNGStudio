#include "messagebox.h"
#include "raylib.h"
#include "str.h"
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <pathbuf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <filedialog.h>
#include <string.h>
#include <sys/stat.h>

#include <raylib-nuklear.h>
#include <unistd.h>

struct new_file {
    char *buffer;
    int len;
    bool is_file;
    bool cleanup;
};

static struct nk_image images[IMG_TYPE_SZ] = {0};
static char img_filter[] = "png;bmp;jpg;jpeg;gif;psd";

static int on_new_file(struct nk_context *ctx, struct messagebox *box);
static int entry_comparar(const void *p1, const void *p2);
static struct dir_entry *append_file(struct filedialog *dialog, const char *name);
static const char *filter_out(const char *filename, const char *filter);
static void init_content(struct filedialog *dialog);
static void deinit_content(struct filedialog *dialog);

void filedialog_init(struct filedialog *dialog, bool write)
{
    filedialog_deinit(dialog);
    dialog->open_for_write = write;
    init_content(dialog);
}

bool filedialog_up(struct filedialog *dialog)
{
    struct path *prev = path_pop(&dialog->current_directory);
    if (prev == NULL && dialog->current_directory.name != NULL) {
        free(dialog->current_directory.name);
        dialog->current_directory.name = NULL;
        dialog->current_directory.is_file = false;

        deinit_content(dialog);
        init_content(dialog);

        return true;
    }

    if (prev == NULL)
        return false;

    deinit_content(dialog);
    init_content(dialog);

    free(prev->name);
    free(prev);

    return true;
}

void filedialog_enter(struct filedialog *dialog, const char *dir)
{
    path_append_dir(&dialog->current_directory, strdup(dir));

    deinit_content(dialog);
    init_content(dialog);
}

size_t filedialog_selsz(const struct filedialog *dialog)
{
    if (dialog->selected_index == -1)
        return 0;

    size_t sz = path_dirsz(&dialog->current_directory);
    struct dir_entry *e = dialog->dir_content + dialog->selected_index;

    return sz + (e->is_file == true ? 0 : 1) + strlen(e->name);
}

void filedialog_selected(const struct filedialog *dialog, size_t selsz,
    char *buf)
{
    if (dialog->selected_index == -1)
        return;

    struct dir_entry *e = dialog->dir_content + dialog->selected_index;

    size_t sz = path_dirsz(&dialog->current_directory);
    path_dir(&dialog->current_directory, selsz, buf);

    buf += sz;
    selsz -= sz;

    sz = sized_strncpy(buf, e->name, selsz);
    buf += sz;
    selsz -= sz;

    if (e->is_file == false && selsz > 0)
        *buf = PATH_SEPARATOR;
}

void filedialog_show(struct filedialog *dialog)
{
    dialog->show = true;
}

void filedialog_run(struct filedialog *dialog, struct nk_context *ctx)
{
    if (dialog->title == NULL)
        dialog->title = ":3";

    if (dialog->geometry.x == 0 && dialog->geometry.y == 0 &&
        dialog->geometry.w == 0 && dialog->geometry.h == 0) {

        int width = GetScreenWidth();
        int height = GetScreenHeight();
        float w = width / 100.0f * 80.0f;
        float h = height / 100.0f * 80.0f;
        float x = (width - w) / 2.0f;
        float y = (height - h) / 2.0f;

        dialog->geometry = nk_rect(x, y, w, h);
    }

    static struct new_file new_filereq = {0};
    static bool show_hiden = false;

    if (dialog->show) {
        if (nk_begin(ctx, dialog->title, dialog->geometry,
                NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE |
                NK_WINDOW_SCALABLE | NK_WINDOW_BORDER)) {
            
            /* title bar*/

            nk_layout_row_template_begin(ctx, 32);
            nk_layout_row_template_push_static(ctx, 32);
            nk_layout_row_template_push_dynamic(ctx);
            nk_layout_row_template_push_static(ctx, 32);
            nk_layout_row_template_end(ctx);

            if (nk_button_image(ctx, images[UP_IMG]))
                filedialog_up(dialog);

            size_t sz = path_dirsz(&dialog->current_directory);
            char buf[sz + 1];
            memset(buf, 0, sz + 1);
            path_dir(&dialog->current_directory, sz, buf);

            int len = sz;

            nk_edit_string(ctx, NK_EDIT_DEACTIVATED, buf, &len, sz, nk_filter_default);

            if (nk_button_image(ctx, images[REFRESH_IMG])) {
                deinit_content(dialog);
                init_content(dialog);
            }

            /* sidebar and file grid */

            struct nk_rect total = nk_window_get_content_region(ctx);

            nk_layout_row_begin(ctx, NK_DYNAMIC, total.h - 80, 1);

            nk_layout_row_push(ctx, NK_UNDEFINED);
            struct nk_rect bounds = nk_layout_space_bounds(ctx);
            bounds.h += 30;

            if (nk_group_begin(ctx, "Files", NK_WINDOW_BORDER)) {
                static bool new_open = false;
                if (nk_contextual_begin(ctx, 0, nk_vec2(256, 256), bounds)) {
                    nk_layout_row_dynamic(ctx, 25, 1);

                    struct nk_vec2 old_padding = ctx->style.window.group_padding;
                    ctx->style.window.group_padding = nk_vec2(0, 0);

                    if (nk_group_begin(ctx, "New", NK_WINDOW_NO_SCROLLBAR)) {
                        struct nk_style_button style = ctx->style.combo.button;
                        style.text_alignment = NK_TEXT_LEFT;
                        nk_layout_row_template_begin(ctx, 25);
                        nk_layout_row_template_push_dynamic(ctx);
                        nk_layout_row_template_push_static(ctx, 25);
                        nk_layout_row_template_end(ctx);

                        if (nk_button_label_styled(ctx, &style, "New") ||
                            nk_button_symbol_styled(ctx, &style, NK_SYMBOL_TRIANGLE_DOWN)){
                            new_open = !new_open;
                        }

                        nk_group_end(ctx);
                    }

                    ctx->style.window.group_padding = old_padding;

                    if (new_open) {
                        nk_layout_row_dynamic(ctx, 2, 1);
                        nk_rule_horizontal(ctx, ctx->style.window.border_color, false);
                        nk_layout_row_dynamic(ctx, 25, 1);
                        if (nk_contextual_item_label(ctx, "File", NK_TEXT_LEFT)) {
                            new_filereq.is_file = true;
                            new_filereq.cleanup = true;
                            dialog->msg_box = messagebox_custom("New File", on_new_file, &new_filereq);
                        }

                        if (nk_contextual_item_label(ctx, "Directory", NK_TEXT_LEFT)) {
                            new_filereq.is_file = false;
                            new_filereq.cleanup = true;
                            dialog->msg_box = messagebox_custom("New Directory", on_new_file, &new_filereq);
                        }
                        nk_layout_row_dynamic(ctx, 2, 1);
                        nk_rule_horizontal(ctx, ctx->style.window.border_color, false);
                        nk_layout_row_dynamic(ctx, 25, 1);
                    }

                    nk_checkbox_label(ctx, "Show hidden files", &show_hiden);

                    nk_contextual_end(ctx);
                } else {
                    new_open = false;
                }

                nk_layout_row_template_begin(ctx, 32);
                nk_layout_row_template_push_static(ctx, 32);
                nk_layout_row_template_push_dynamic(ctx);
                nk_layout_row_template_end(ctx);

                for (size_t i = 0; i < dialog->content_size; i++) {
                    struct dir_entry *e = dialog->dir_content + i;
                    if (i != dialog->selected_index)
                        e->selected = false;

                    if (e->hidden && !show_hiden)
                        continue;

                    nk_bool prev = e->selected;
                    enum image_type type = DIR_IMG;

                    if (e->is_file)
                        type++;

                    if (filter_out(e->name, img_filter) != NULL)
                        type = IMG_IMG;

                    nk_image(ctx, images[type]);
                    nk_selectable_label(ctx, e->name, NK_TEXT_LEFT, &e->selected);

                    if (e->selected) {
                        dialog->selected_index = i;
                    }

                    if (prev == true && e->selected == false) {
                        if (!e->is_file) {
                            filedialog_enter(dialog, e->name);
                            goto end;
                        } else {
                            if (dialog->selected_index != -1)
                                dialog->show = false;
                        }
                    }
                }
end:
                nk_group_end(ctx);
            }

            nk_layout_row_end(ctx);
            nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
            nk_layout_row_push(ctx, 0.7f);

            const char *selected = "";
            if (dialog->selected_index != -1)
                selected = dialog->dir_content[dialog->selected_index].name;

            len = strlen(selected) + 1;

            nk_edit_string(ctx, NK_EDIT_DEACTIVATED, (char*) selected, &len, len, nk_filter_default);
            nk_widget_disable_end(ctx);
            nk_layout_row_push(ctx, 0.2f);

            struct nk_vec2 new_size = nk_widget_size(ctx);

            const char *filter = NULL;
            if (dialog->filter != NULL)
                filter = dialog->filter;

            if (filter != NULL && nk_combo_begin_label(ctx, "Filters", nk_vec2(new_size.x, 100))) {
                nk_layout_row_dynamic(ctx, 30, 1);
                const char *iter = filter;
                do {
                    const char *prev = iter;
                    if (*prev == 0)
                        break;

                    iter = strchrnul(iter, ';');
                    size_t len = iter - prev;
                    char split[len + 1];
                    memset(split, 0, len + 1);
                    memcpy(split, prev, len);
                    nk_label(ctx, split, NK_TEXT_LEFT);
                } while (iter++);

                nk_combo_end(ctx);
            } else {
                if (filter == NULL)
                    nk_label(ctx, "Every File", NK_TEXT_CENTERED);
            }

            nk_layout_row_push(ctx, 0.1f);
            if (nk_button_label(ctx, "Select")) {
                if (dialog->selected_index != -1) {
                    struct dir_entry *e = dialog->dir_content +
                        dialog->selected_index;

                    if (!e->is_file)
                        filedialog_enter(dialog, e->name);
                    else
                        dialog->show = false;
                } else
                    dialog->msg_box = messagebox_error("Select a File",
                        "Please select a file!");
            }

            nk_layout_row_end(ctx);

            messagebox_run(&dialog->msg_box, ctx);

            if (dialog->msg_box.userdata == &new_filereq) {
                if (dialog->msg_box.res == 1) {
                    path_append_file(&dialog->current_directory,strdup(new_filereq.buffer));

                    size_t sz = path_bufsz(&dialog->current_directory);
                    char tmpbuf[sz + 1];
                    memset(tmpbuf, 0, sz + 1);
                    path_str(&dialog->current_directory, sz, tmpbuf);

                    if (new_filereq.is_file) {
                        int fd = open(tmpbuf, O_CREAT, DEFFILEMODE);

                        filedialog_up(dialog);

                        if (fd == -1) {
                            if (errno == EACCES) {
                                dialog->msg_box =
                                    messagebox_error("Error", "Permissions Denied!");
                            } else {
                                perror("open");
                                abort();
                            }
                        } else {
                            close(fd);
                        }
                    } else {
                        int res = mkdir(tmpbuf, 0755);
                        filedialog_up(dialog);

                        if (res == -1) {
                            switch (errno) {
                            case EACCES:
                                dialog->msg_box =
                                    messagebox_error("Error", "Permissions Denied!");
                                break;
                            case EEXIST:
                                dialog->msg_box =
                                    messagebox_error("Error", "Directory Exists!");
                                break;
                            default:
                                perror("mkdir");
                                abort();
                            }
                        }
                    }

                    dialog->msg_box.userdata = NULL;
                }
            }
        } else {
            struct nk_vec2 wprop = nk_window_get_position(ctx);
            dialog->geometry.x = wprop.x;
            dialog->geometry.y = wprop.y;

            wprop = nk_window_get_size(ctx);

            dialog->geometry.w = wprop.x;
            dialog->geometry.h = wprop.y;

            dialog->show = false;
        }

        nk_end(ctx);
    }
}

void filedialog_deinit(struct filedialog *dialog)
{
    path_deinit(&dialog->current_directory, true);
    dialog->current_directory.is_file = false;
    dialog->current_directory.name = NULL;

    deinit_content(dialog);

    dialog->open_for_write = false;
    dialog->show = false;

    dialog->geometry = nk_rect(0, 0, 0, 0);
}

static void init_content(struct filedialog *dialog)
{
    size_t sz = path_dirsz(&dialog->current_directory);
    char buf[sz + 1];
    memset(buf, 0, sz + 1);
    path_dir(&dialog->current_directory, sz, buf);

    int fd = open(buf, O_RDONLY | O_DIRECTORY);

    if (fd == -1) {
        if (errno == EACCES) {
            dialog->msg_box = messagebox_error("Error", "Permissions Denied!");
            return;
        }

        printf("About to abort on path: %s\n", buf);
        perror("open");
        abort();
    }

    DIR *dir = fdopendir(fd);

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len == 1 || len == 2) {
            if (len == 1 && *entry->d_name == '.')
                continue;

            if (*entry->d_name == '.' && entry->d_name[1] == '.')
                continue;
        }

        switch (entry->d_type) {
        case DT_DIR:
            append_file(dialog, entry->d_name)->is_file = false;
            break;
        case DT_REG: {
            const char *out = filter_out(entry->d_name, dialog->filter);

            if (out != NULL)
                append_file(dialog, out);

            break;
        }
        case DT_UNKNOWN: {
            struct stat s;
            if (lstat(entry->d_name, &s) == -1) {
                perror("lstat");
                abort();
            }

            if (S_ISDIR(s.st_mode)) {
                append_file(dialog, entry->d_name)->is_file = false;
            } else if (S_ISREG(s.st_mode)) {
                const char *out = filter_out(entry->d_name, dialog->filter);

                if (out != NULL)
                    append_file(dialog, out);
            }

            break;
        }
        default:
            continue;
        }
    }

    /* sort */
    qsort(dialog->dir_content, dialog->content_size, sizeof(struct dir_entry), entry_comparar);

    closedir(dir);
}

static void deinit_content(struct filedialog *dialog)
{
    if (dialog->dir_content != NULL) {
        for (size_t i = 0; i < dialog->content_size; i++) {
            free(dialog->dir_content[i].name);
        }

        free(dialog->dir_content);
    }

    dialog->content_size = 0;
    dialog->dir_content = NULL;
    dialog->selected_index = -1;
}

static const char *filter_out(const char *filename, const char *filter)
{
    const char *iter = filter;
    const char *ext = strrchr(filename, '.');

    if (filter == NULL)
        return filename;

    if (ext == NULL)
        return NULL;

    ext++;

    do {
        char *next = strchrnul(iter, ';');

        *next = 0;

        if (strcmp(ext, iter) == 0)
            return filename;

        iter = next + 1;

        if (*iter == 0)
            break;

        *next = ';';
    } while (iter != NULL);

    return NULL;
}

static struct dir_entry *append_file(struct filedialog *dialog, const char *name)
{
    dialog->dir_content = realloc(
        dialog->dir_content, sizeof(struct dir_entry) * ++dialog->content_size);

    struct dir_entry *e = dialog->dir_content + (dialog->content_size - 1);
    e->selected = false;
    e->is_file = true;
    e->hidden = false;
    e->name = strdup(name);

    if (*name == '.')
        e->hidden = true;

    return e;
}

void filedialog_register_icon(enum image_type type, struct nk_image img)
{
    images[type] = img;
}

static int entry_comparar(const void *p1, const void *p2)
{
    const struct dir_entry *e1 = p1;
    const struct dir_entry *e2 = p2;

    if (e1->is_file != e2->is_file)
        return e1->is_file - e2->is_file;

    return strcmp(e1->name, e2->name);
}

static int on_new_file(struct nk_context *ctx, struct messagebox *box)
{
    struct new_file *req = box->userdata;

    if (req->cleanup) {
        if (req->buffer != NULL) {
            free(req->buffer);
        }
        req->cleanup = false;
        req->buffer = malloc(1);
        *req->buffer = 0;
        req->len = 0;
    }

    int new_len = req->len;

    nk_layout_row_dynamic(ctx, 25, 1);
    nk_label(ctx, "Enter name: ", NK_TEXT_LEFT);
    nk_edit_string(ctx, NK_EDIT_SIMPLE, req->buffer, &new_len, req->len + 2, nk_filter_default);

    if (new_len > req->len) {
        req->buffer = realloc(req->buffer, ++req->len + 1);
        req->buffer[req->len] = 0;
    } else if (new_len < req->len) {
        req->buffer[new_len] = 0;
        req->len--;
    }

    struct nk_rect bounds = nk_window_get_content_region(ctx);

    bounds.h -= 25 * 4;

    nk_layout_row_dynamic(ctx, bounds.h, 1);
    nk_label(ctx, " ", NK_TEXT_LEFT);

    nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 3);
    nk_layout_row_push(ctx, 0.7f);
    nk_label(ctx, " ", NK_TEXT_LEFT);

    nk_layout_row_push(ctx, 0.15f);
    if (nk_button_label(ctx, "Cancel"))
        return 0;

    nk_layout_row_push(ctx, 0.14f);
    if (nk_button_label(ctx, "Ok"))
        return 1;

    return -1;
}