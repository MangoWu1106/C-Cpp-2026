#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAX_GOODS 100
#define MODEL_SIZE 64
#define DATA_FILE "warehouse.txt"

typedef struct {
    char model[MODEL_SIZE];
    int quantity;
} Goods;

static Goods warehouse[MAX_GOODS];
static int goods_count = 0;
static char data_file[sizeof(__FILE__) + sizeof(DATA_FILE)];

static void setup_data_file(void) {
    /* CLion/CMake compiles with an absolute source path in __FILE__. */
    strcpy(data_file, __FILE__);
    char *separator = strrchr(data_file, '/');
    char *backslash = strrchr(data_file, '\\');
    if (backslash != NULL && (separator == NULL || backslash > separator)) {
        separator = backslash;
    }
    strcpy(separator != NULL ? separator + 1 : data_file, DATA_FILE);
}

/* Return 1 for a complete line, 0 for EOF, -1 for an overlong line. */
static int read_line(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return 0;
    }
    if (strchr(buffer, '\n') == NULL && !feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        return -1;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 1;
}

/* Reject text, overflow and numbers outside the requested range. */
static int parse_integer(const char *text, int minimum, int maximum, int *value) {
    char *end;
    errno = 0;
    long number = strtol(text, &end, 10);
    if (end == text || errno == ERANGE) {
        return 0;
    }
    while (isspace((unsigned char)*end)) {
        ++end;
    }
    if (*end != '\0' || number < minimum || number > maximum) {
        return 0;
    }
    *value = (int)number;
    return 1;
}

static int read_integer(const char *prompt, int minimum, int maximum, int *value) {
    char line[128];
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        int result = read_line(line, sizeof(line));
        if (result == 0) {
            return 0;
        }
        if (result == 1 && parse_integer(line, minimum, maximum, value)) {
            return 1;
        }
        printf("输入无效，请输入 %d 到 %d 之间的整数。\n", minimum, maximum);
    }
}

static int read_model(char *model) {
    char line[128];
    for (;;) {
        printf("请输入货物型号（不含空格，最多 %d 字节）：", MODEL_SIZE - 1);
        fflush(stdout);
        int result = read_line(line, sizeof(line));
        if (result == 0) {
            return 0;
        }
        int valid = result == 1 && line[0] != '\0' && strlen(line) < MODEL_SIZE;
        for (int i = 0; valid && line[i] != '\0'; ++i) {
            if (isspace((unsigned char)line[i])) {
                valid = 0;
            }
        }
        if (valid) {
            strcpy(model, line);
            return 1;
        }
        puts("型号无效，请重新输入。");
    }
}

static int find_goods(const char *model) {
    for (int i = 0; i < goods_count; ++i) {
        if (strcmp(warehouse[i].model, model) == 0) {
            return i;
        }
    }
    return -1;
}

static int is_ascii(const char *text) {
    for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; ++p) {
        if (*p >= 128) {
            return 0;
        }
    }
    return 1;
}

static void show_goods(void) {
    if (goods_count == 0) {
        puts("当前没有存货记录。");
        return;
    }
    /* Unicode glyph widths depend on the viewer's font. A vertical layout
       keeps Chinese and other Unicode model names readable without padding. */
    for (int i = 0; i < goods_count; ++i) {
        if (!is_ascii(warehouse[i].model)) {
            puts("\n===== 存货列表 =====");
            for (int j = 0; j < goods_count; ++j) {
                printf("序号：%d\n型号：%s\n数量：%d\n\n",
                       j + 1, warehouse[j].model, warehouse[j].quantity);
            }
            return;
        }
    }

    /* ASCII-only columns avoid mixing Chinese and Latin font metrics. */
    int index_width = (int)strlen("No.");
    int model_width = (int)strlen("Model");
    int quantity_width = (int)strlen("Quantity");
    char index_text[16], quantity_text[16];
    for (int i = 0; i < goods_count; ++i) {
        int width = snprintf(index_text, sizeof(index_text), "%d", i + 1);
        if (width > index_width) { index_width = width; }
        width = (int)strlen(warehouse[i].model);
        if (width > model_width) { model_width = width; }
        width = snprintf(quantity_text, sizeof(quantity_text), "%d", warehouse[i].quantity);
        if (width > quantity_width) { quantity_width = width; }
    }
    puts("\nNo. = 序号；Model = 型号；Quantity = 数量");
    printf("%-*s  %-*s  %*s\n", index_width, "No.", model_width, "Model",
           quantity_width, "Quantity");
    for (int i = 0; i < goods_count; ++i) {
        printf("%-*d  %-*s  %*d\n", index_width, i + 1,
               model_width, warehouse[i].model, quantity_width, warehouse[i].quantity);
    }
}

/* Return 0 if input ends, so main can save before exiting. */
static int stock_in(void) {
    char model[MODEL_SIZE];
    int quantity;
    if (!read_model(model)) {
        return 0;
    }
    int index = find_goods(model);
    if (index < 0 && goods_count == MAX_GOODS) {
        puts("货物种类已满，无法添加新型号。");
        return 1;
    }
    if (!read_integer("请输入入库数量：", 1, INT_MAX, &quantity)) {
        return 0;
    }
    if (index < 0) {
        index = goods_count++;
        strcpy(warehouse[index].model, model);
        warehouse[index].quantity = 0;
    }
    if (quantity > INT_MAX - warehouse[index].quantity) {
        puts("数量过大，入库失败。");
        return 1;
    }
    warehouse[index].quantity += quantity;
    printf("入库成功，型号 %s 当前库存：%d\n", model, warehouse[index].quantity);
    return 1;
}

static int stock_out(void) {
    char model[MODEL_SIZE];
    int quantity;
    if (!read_model(model)) {
        return 0;
    }
    int index = find_goods(model);
    if (index < 0) {
        puts("没有找到该型号。");
        return 1;
    }
    if (!read_integer("请输入出库数量：", 1, INT_MAX, &quantity)) {
        return 0;
    }
    if (quantity > warehouse[index].quantity) {
        printf("库存不足，当前库存：%d\n", warehouse[index].quantity);
        return 1;
    }
    warehouse[index].quantity -= quantity;
    printf("出库成功，型号 %s 当前库存：%d\n", model, warehouse[index].quantity);
    return 1;
}

static int load_goods(void) {
    /* a+ creates a missing file immediately, before showing the first menu. */
    FILE *file = fopen(data_file, "a+");
    if (file == NULL) {
        perror("无法读取库存文件");
        return 0;
    }
    rewind(file);
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        char model[256], number[32], extra;
        int quantity;
        int fields = sscanf(line, "%255s %31s %c", model, number, &extra);
        if (fields == EOF) {
            continue; /* Allow blank lines. */
        }
        if (fields != 2 || strlen(model) >= MODEL_SIZE
            || !parse_integer(number, 0, INT_MAX, &quantity)
            || goods_count == MAX_GOODS || find_goods(model) >= 0) {
            fputs("库存文件格式错误，停止运行以避免覆盖原数据。\n", stderr);
            fclose(file);
            return 0;
        }
        strcpy(warehouse[goods_count].model, model);
        warehouse[goods_count++].quantity = quantity;
    }
    int failed = ferror(file);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        fputs("读取库存文件失败。\n", stderr);
        return 0;
    }
    printf("已读取 %d 种货物。\n", goods_count);
    return 1;
}

static int save_goods(void) {
    FILE *file = fopen(data_file, "w");
    if (file == NULL) {
        perror("无法保存库存文件");
        return 0;
    }
    int failed = 0;
    for (int i = 0; i < goods_count; ++i) {
        if (fprintf(file, "%s %d\n", warehouse[i].model, warehouse[i].quantity) < 0) {
            failed = 1;
            break;
        }
    }
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        fputs("保存库存失败，请检查磁盘空间和文件权限。\n", stderr);
        return 0;
    }
    printf("库存已保存到 %s。\n", data_file);
    return 1;
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    setup_data_file();
    printf("库存文件：%s\n", data_file);
    if (!load_goods()) {
        return EXIT_FAILURE;
    }
    int running = 1;
    while (running) {
        int choice;
        puts("\n===== 简单进销存 =====");
        puts("1. 显示存货列表\n2. 入库\n3. 出库\n4. 退出程序");
        if (!read_integer("请选择（1-4）：", 1, 4, &choice)) {
            break;
        }
        switch (choice) {
            case 1: show_goods(); break;
            case 2: running = stock_in(); break;
            case 3: running = stock_out(); break;
            case 4: running = 0; break;
        }
    }
    return save_goods() ? EXIT_SUCCESS : EXIT_FAILURE;
}
