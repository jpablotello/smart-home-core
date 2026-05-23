#pragma once

/**
 * @brief Mock cJSON Header
 * This is a minimal mock implementation to allow compilation.
 * For production use, ensure cJSON is available or install esp-json or similar library.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mock cJSON structure and functions
typedef struct cJSON {
    struct cJSON *next, *prev;
    struct cJSON *child;
    int type;
    char *valuestring;
    int valueint;
    double valuedouble;
    char *string;
} cJSON;

// Mock function declarations
static inline cJSON *cJSON_ParseWithLength(const char *value, size_t len) {
    return NULL;  // Stub
}

static inline cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    return NULL;  // Stub
}

static inline int cJSON_IsString(const cJSON *item) {
    return 0;  // Stub
}

static inline int cJSON_IsNumber(const cJSON *item) {
    return 0;  // Stub
}

static inline void cJSON_Delete(cJSON *c) {
    // Stub
}

static inline cJSON *cJSON_CreateObject(void) {
    return NULL;  // Stub
}

static inline cJSON *cJSON_AddStringToObject(cJSON *object, const char *name, const char *string) {
    return NULL;  // Stub
}

static inline cJSON *cJSON_AddNumberToObject(cJSON *object, const char *name, double number) {
    return NULL;  // Stub
}

static inline char *cJSON_PrintUnformatted(cJSON *item) {
    return NULL;  // Stub
}

#ifdef __cplusplus
}
#endif
