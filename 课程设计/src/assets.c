#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "assets.h"

unsigned char *assets_read_font(int *size)
{
    wchar_t path[32768];
    const wchar_t suffix[] = L"assets\\NotoSansCJKsc-Regular.otf";
    DWORD length = GetModuleFileNameW(NULL, path, ARRAYSIZE(path));
    *size = 0;
    if (length == 0 || length >= ARRAYSIZE(path)) {
        return NULL;
    }

    wchar_t *directory_end = wcsrchr(path, L'\\');
    if (!directory_end || ARRAYSIZE(path) - (size_t)(directory_end + 1 - path) < ARRAYSIZE(suffix)) {
        return NULL;
    }
    wmemcpy(directory_end + 1, suffix, ARRAYSIZE(suffix));

    FILE *file = _wfopen(path, L"rb");
    if (!file) {
        return NULL;
    }
    long file_size = fseek(file, 0, SEEK_END) == 0 ? ftell(file) : -1;
    if (file_size <= 0 || file_size > INT_MAX || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    unsigned char *data = malloc((size_t)file_size);
    if (!data || fread(data, 1, (size_t)file_size, file) != (size_t)file_size) {
        free(data);
        fclose(file);
        return NULL;
    }

    fclose(file);
    *size = (int)file_size;
    return data;
}

void assets_show_error(const wchar_t *message)
{
    MessageBoxW(NULL, message, L"五子棋", MB_OK | MB_ICONERROR);
}
