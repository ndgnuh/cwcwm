#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cwc/util.h"

#define KEY_LEN 10

static int TABLE_SIZE;

char *number_to_string(int n)
{
    char *container = calloc(1, KEY_LEN);
    sprintf(container, "%d", n);

    return container;
}

// setup data for repeated_read
void setup_data(struct cwc_hhmap *m)
{
    srand(69);
    for (int i = 0; i < TABLE_SIZE; i++) {
        // char key[KEY_LEN];
        // sprintf(key, "%d", i);
        char *key = number_to_string(i);
        cwc_hhmap_ninsert(m, key, KEY_LEN, key);
    }
}

void destroy_data(struct cwc_hhmap *m)
{
    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        cwc_hhmap_nremove(m, key, KEY_LEN);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        char *val = cwc_hhmap_nget(m, key, KEY_LEN);
        assert(val == NULL);
    }
}

void basic_perf(struct cwc_hhmap *m)
{
    // basic performance test
    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        cwc_hhmap_ninsert(m, key, KEY_LEN, (void *)(long)i);
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        char *value = cwc_hhmap_nget(m, key, KEY_LEN);
        // printf("%d %ld %ld\n", i, m->size, m->alloc);
        assert(value == i);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        cwc_hhmap_nremove(m, key, KEY_LEN);
        // printf("%d %ld %ld\n", i, m->size, m->alloc);
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        char *value = cwc_hhmap_nget(m, key, KEY_LEN);
        assert(value == NULL);
    }
}

void basic_operation(struct cwc_hhmap *m)
{
    char key[10] = "key";
    cwc_hhmap_insert(m, "key0", "value0");
    cwc_hhmap_insert(m, "key1", "value1");
    cwc_hhmap_insert(m, "key2", "value2");
    cwc_hhmap_insert(m, "key3", "value3");
    cwc_hhmap_insert(m, "key4", "value4");
    cwc_hhmap_insert(m, "key5", "value5");

    for (int i = 0; i < 8; i++) {
        sprintf(key + 3, "%d", i);
        char *value = cwc_hhmap_get(m, key);
        printf("%p\n", value);
    }

    for (int i = 0; i < 8; i++) {
        sprintf(key + 3, "%d", i);
        cwc_hhmap_remove(m, key);
        char *value = cwc_hhmap_get(m, key);
        printf("%p\n", value);
    }
}

void repeated_read(struct cwc_hhmap *m)
{
    // seq read
    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", i);
        char *val = cwc_hhmap_nget(m, key, KEY_LEN);
        // assert(strcmp(val, key) == 0);
    }

    // rand read
    for (int i = 0; i < TABLE_SIZE; i++) {
        char key[KEY_LEN];
        memset(key, 0, KEY_LEN);
        sprintf(key, "%d", rand() % TABLE_SIZE);
        char *val = cwc_hhmap_nget(m, key, KEY_LEN);
        // assert(strcmp(val, key) == 0);
    }
}

int main()
{
    TABLE_SIZE          = 1e7;
    struct cwc_hhmap *m = cwc_hhmap_create(0);
    setup_data(m);

    for (int i = 0; i < 5; i++) {
        // basic_perf(m);
        repeated_read(m);
    }

    destroy_data(m);

    cwc_hhmap_destroy(m);
    return 0;
}
