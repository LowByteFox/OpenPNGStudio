#ifndef _FILEDIALOG_H_
#define _FILEDIALOG_H_

#include <pathbuf.h>
#include <stdbool.h>
#include <stddef.h>

struct dir_entry {
    char *name;
    bool is_file;
};

struct filedialog {
    /* STATE */
    struct path current_directory;
    struct dir_entry *dir_content;
    size_t content_size;
    int selected_index;
    unsigned int row_width;
    bool open_for_write;
    bool show;

    /* CFG */
    const char *title;
    const char *filter;
};

void filedialog_init(struct filedialog *dialog, bool write);
/* returns false when in the root of the FS */
bool filedialog_up(struct filedialog *dialog);
void filedialog_enter(struct filedialog *dialog, const char *dir);

/* returns the length of the selected file */
size_t filedialog_selsz(const struct filedialog *dialog);

void filedialog_selected(const struct filedialog *dialog, size_t selsz,
    char *buf);

/* once everything is setup, trigger opening */
void filedialog_show(struct filedialog *dialog);

void filedialog_run(struct filedialog *dialog);

void filedialog_deinit(struct filedialog *dialog);

#endif