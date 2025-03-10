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

#include <core/str.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <core/pathbuf.h>

void path_append_dir(struct path *path, char *new_path)
{
    assert(path != NULL);

    if (path->name == NULL) {
        path->name = new_path;
        return;
    }

    struct path *iter = path;
    for (; iter->next != NULL; iter = iter->next);

    iter->next = calloc(1, sizeof(*path));
    if (iter->next == NULL)
        abort();

    assert(iter->is_file == false);

    iter->next->name = new_path;
}

void path_append_file(struct path *path, char *filename)
{
    assert(path != NULL);

    if (path->name == NULL) {
        path->name = filename;
        path->is_file = true;
        return;
    }

    struct path *iter = path;
    for (; iter->next != NULL; iter = iter->next);

    iter->next = calloc(1, sizeof(*path));
    if (iter->next == NULL)
        abort();

    assert(iter->is_file == false);

    iter->next->name = filename;
    iter->next->is_file = true;
}

struct path *path_pop(struct path *path)
{
    if (path->next == NULL) {
        return NULL;
    }

    struct path *prev = path;
    struct path *iter = path;

    while (iter->next != NULL) {
        prev = iter;
        iter = iter->next;
    }

    prev->next = NULL;

    return iter;
}

size_t path_bufsz(const struct path *path)
{
    assert(path != NULL);

#ifndef _WIN32
    size_t bufsz = 1; /* separator length is 1 */
#else
    size_t bufsz = 3; /* include drive letter */
#endif

    const struct path *iter = path;

    while (iter != NULL) {
        if (iter->name == NULL)
            break;

        bufsz += strlen(iter->name);

        if (!iter->is_file)
            bufsz++; /* separator */

        iter = iter->next;
    }

    return bufsz + 1;
}

size_t path_dirsz(const struct path *path)
{
    assert(path != NULL);

#ifndef _WIN32
    size_t bufsz = 1; /* separator length is 1 */
#else
    size_t bufsz = 3; /* include drive letter */
#endif

    const struct path *iter = path;

    while (iter != NULL && !iter->is_file) {
        if (iter->name == NULL)
            break;

        bufsz += strlen(iter->name);
        bufsz++; /* separator */

        iter = iter->next;
    }

    return bufsz + 1;
}

const char *path_basename(const struct path *path)
{
    const struct path *iter = path;

    while (iter->next != NULL)
        iter = iter->next;

    return iter->name;
}

void path_dir(const struct path *path, size_t dir_bufsz, char *buf)
{
    assert(path != NULL);

#ifndef _WIN32
    *buf = PATH_SEPARATOR;
#else
    memcpy(buf, "C:\\", 3); /* drive letter will be C most of the time */
    buf += 2;
#endif

    buf++;
    dir_bufsz--;

    const struct path *iter = path;

    while (iter != NULL && dir_bufsz > 0 && !iter->is_file) {
        if (iter->name == NULL)
            break;

        size_t wrote = sized_strncpy(buf, iter->name, dir_bufsz);
        dir_bufsz -= wrote;

        buf += wrote;

        if (dir_bufsz <= 0)
            return;

        dir_bufsz -= 1; /* separator */
        *buf = PATH_SEPARATOR;
        buf++;

        iter = iter->next;
    }
}

void path_str(const struct path *path, size_t path_bufsz, char *buf)
{
    assert(path != NULL);

#ifndef _WIN32
    *buf = PATH_SEPARATOR;
#else
    memcpy(buf, "C:\\", 3); /* drive letter will be C most of the time */
    buf += 2;
#endif

    buf++;
    path_bufsz--;

    const struct path *iter = path;

    while (iter != NULL && path_bufsz > 0) {
        if (iter->is_file)
            path_bufsz--; /* no separator is next */

        size_t wrote = sized_strncpy(buf, iter->name, path_bufsz);
        path_bufsz -= wrote;

        buf += wrote;

        if (iter->is_file)
            return;

        if (path_bufsz <= 0)
            return;

        path_bufsz -= 1; /* separator */
        *buf = PATH_SEPARATOR;
        buf++;

        iter = iter->next;
    }
}

void path_deinit(struct path *path, bool free_strings)
{
    assert(path != NULL);

    struct path *iter = path->next;

    while (iter != NULL) {
        struct path *prev = iter;
        iter = iter->next;

        if (free_strings)
            free(prev->name);

        free(prev);
    }

    if (free_strings)
        free(path->name);

    path->next = NULL;
}
