#include "file.h"

#include <stdlib.h>
#include <string.h>

#include "util/log.h"

char *
sc_file_get_local_path(const char *name) {
    char *executable_path = sc_file_get_executable_path();
    if (!executable_path) {
        return NULL;
    }

    // dirname() does not work correctly everywhere, so get the parent
    // directory manually.
    // See <https://github.com/Genymobile/scrcpy/issues/2619>
    char *p = strrchr(executable_path, SC_PATH_SEPARATOR);
    if (!p) {
        LOGE("Unexpected executable path: \"%s\" (it should contain a '%c')",
             executable_path, SC_PATH_SEPARATOR);
        free(executable_path);
        return NULL;
    }

    *p = '\0'; // modify executable_path in place
    char *dir = executable_path;
    size_t dirlen = strlen(dir);
    size_t namelen = strlen(name);

    size_t len = dirlen + namelen + 2; // +2: '/' and '\0'
    char *file_path = malloc(len);
    if (!file_path) {
        LOG_OOM();
        free(executable_path);
        return NULL;
    }

    memcpy(file_path, dir, dirlen);
    file_path[dirlen] = SC_PATH_SEPARATOR;
    // namelen + 1 to copy the final '\0'
    memcpy(&file_path[dirlen + 1], name, namelen + 1);

    free(executable_path);

    return file_path;
}

char *sc_file_read(const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    char *content = NULL;
    size_t read_chars = 0;

    /* open in read binary mode */
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }
    length = ftell(file);
    if (length < 0)
    {
        goto cleanup;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* allocate content buffer */
    content = (char *)malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto cleanup;
    }

    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        free(content);
        content = NULL;
        goto cleanup;
    }
    content[read_chars] = '\0';

cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    return content;
}