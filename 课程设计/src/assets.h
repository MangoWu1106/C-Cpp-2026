#ifndef GOBANG_ASSETS_H
#define GOBANG_ASSETS_H

#include <stddef.h>

unsigned char *assets_read_font(int *size);
void assets_show_error(const wchar_t *message);

#endif
