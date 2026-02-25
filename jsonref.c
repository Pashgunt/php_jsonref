#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_jsonref.h"
#include "jsonref.h"
#include <jansson.h>
#include <string.h>
#include <time.h>
#include "jsonref_arginfo.h"

static int le_compiled_path;
#define LE_COMPILED_PATH "compiled_path"
#define UNUSED(x) (void)(x)

typedef struct {
    char *json_str;
    size_t json_len;
    json_t *root;
    int refcount;
    time_t last_used;
} json_cache_entry_t;

#define JSON_CACHE_SIZE 10
static json_cache_entry_t json_cache[JSON_CACHE_SIZE];

typedef struct {
    char *path;
    int depth;
    char **tokens;
    int refcount;
} compiled_path_t;

#define COMPILED_PATH_CACHE_SIZE 20
static compiled_path_t *path_cache[COMPILED_PATH_CACHE_SIZE];

static void php_jsonref_compiled_path_dtor(zend_resource *rsrc)
{
    compiled_path_t *comp = (compiled_path_t*)rsrc->ptr;
    if (comp) {
        if (comp->path) efree(comp->path);

        for (int i = 0; i < comp->depth; i++) {
            if (comp->tokens[i]) efree(comp->tokens[i]);
        }

        if (comp->tokens) efree(comp->tokens);

        efree(comp);
    }
}

PHP_MINIT_FUNCTION(jsonref)
{
    UNUSED(type);

    le_compiled_path = zend_register_list_destructors_ex(
        php_jsonref_compiled_path_dtor,
        NULL,
        LE_COMPILED_PATH,
        module_number
    );

    return SUCCESS;
}

static void init_cache(void) {
    static int initialized = 0;
    if (!initialized) {
        memset(json_cache, 0, sizeof(json_cache));
        memset(path_cache, 0, sizeof(path_cache));
        initialized = 1;
    }
}

static json_t* get_cached_json(const char *json_str, size_t json_len) {
    init_cache();

    for (int i = 0; i < JSON_CACHE_SIZE; i++) {
        if (json_cache[i].json_str &&
            json_cache[i].json_len == json_len &&
            memcmp(json_cache[i].json_str, json_str, json_len) == 0) {
            json_cache[i].refcount++;
            json_cache[i].last_used = time(NULL);
            return json_cache[i].root;
        }
    }

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        return NULL;
    }

    int lru_slot = 0;
    time_t oldest_time = time(NULL);

    for (int i = 0; i < JSON_CACHE_SIZE; i++) {
        if (!json_cache[i].json_str) {
            lru_slot = i;
            break;
        }

        if (json_cache[i].last_used < oldest_time) {
            oldest_time = json_cache[i].last_used;
            lru_slot = i;
        }
    }

    if (json_cache[lru_slot].json_str) {
        free(json_cache[lru_slot].json_str);
        json_decref(json_cache[lru_slot].root);
    }

    json_cache[lru_slot].json_str = estrdup(json_str);
    json_cache[lru_slot].json_len = json_len;
    json_cache[lru_slot].root = root;
    json_cache[lru_slot].refcount = 1;
    json_cache[lru_slot].last_used = time(NULL);

    return root;
}

static void cleanup_json_cache(void) {
    for (int i = 0; i < JSON_CACHE_SIZE; i++) {
        if (json_cache[i].json_str) {
            free(json_cache[i].json_str);
            json_decref(json_cache[i].root);
            memset(&json_cache[i], 0, sizeof(json_cache_entry_t));
        }
    }
}

typedef struct {
    char **tokens;
    size_t count;
} path_tokens_t;

static path_tokens_t split_path_fast(const char *path) {
    path_tokens_t result;

    size_t token_count = 1;
    for (const char *c = path; *c; c++) {
        if (*c == '.') token_count++;
    }

    result.count = token_count;

    result.tokens = emalloc(sizeof(char *) * token_count);

    size_t n = 0;
    const char *start = path;

    for (const char *c = path; *c; c++) {
        if (*c == '.') {
            size_t len = c - start;

            if (len > 0) {
                result.tokens[n] = estrndup(start, len);
            } else {
                result.tokens[n] = estrdup("");
            }

            n++;
            start = c + 1;
        }
    }

    size_t last_len = strlen(start);

    if (last_len > 0) {
        result.tokens[n] = estrndup(start, last_len);
    } else {
        result.tokens[n] = estrdup("");
    }

    return result;
}

static void free_path_tokens(path_tokens_t *tokens) {
    if (!tokens || !tokens->tokens) return;

    for (size_t i = 0; i < tokens->count; i++) {
        if (tokens->tokens[i]) {
            efree(tokens->tokens[i]);
        }
    }

    efree(tokens->tokens);
    tokens->tokens = NULL;
}

static compiled_path_t* create_compiled_path(const char *path) {
    compiled_path_t *comp = emalloc(sizeof(compiled_path_t));
    comp->path = estrdup(path);
    comp->refcount = 1;

    path_tokens_t tokens = split_path_fast(path);
    comp->depth = tokens.count;
    comp->tokens = emalloc(sizeof(char *) * comp->depth);

    for (size_t i = 0; i < tokens.count; i++) {
        comp->tokens[i] = estrdup(tokens.tokens[i]);
    }

    free_path_tokens(&tokens);

    return comp;
}

static void json_to_zval(zval *zv, json_t *json) {
    if (!json) {
        ZVAL_NULL(zv);
        return;
    }

    switch (json_typeof(json)) {
        case JSON_STRING:
            ZVAL_STRING(zv, json_string_value(json));
            break;
        case JSON_INTEGER:
            ZVAL_LONG(zv, json_integer_value(json));
            break;
        case JSON_REAL:
            ZVAL_DOUBLE(zv, json_real_value(json));
            break;
        case JSON_TRUE:
            ZVAL_BOOL(zv, 1);
            break;
        case JSON_FALSE:
            ZVAL_BOOL(zv, 0);
            break;
        case JSON_NULL:
            ZVAL_NULL(zv);
            break;
        case JSON_OBJECT:
        case JSON_ARRAY:
        default: {
            char *dump = json_dumps(json, JSON_COMPACT | JSON_ENCODE_ANY);
            ZVAL_STRING(zv, dump);
            free(dump);
            break;
        }
    }
}

static json_t* zval_to_json(zval *zv) {
    switch (Z_TYPE_P(zv)) {
        case IS_STRING:
            return json_string(Z_STRVAL_P(zv));
        case IS_LONG:
            return json_integer(Z_LVAL_P(zv));
        case IS_DOUBLE:
            return json_real(Z_DVAL_P(zv));
        case IS_TRUE:
            return json_true();
        case IS_FALSE:
            return json_false();
        case IS_NULL:
            return json_null();
        default:
            return NULL;
    }
}

static int apply_json_update(json_t *root, const char *path, zval *value) {
    if (!root || !path || !value) return FAILURE;

    path_tokens_t tokens = split_path_fast(path);

    if (tokens.count == 0) {
        free_path_tokens(&tokens);
        return FAILURE;
    }

    json_t *current = root;

    for (size_t i = 0; i < tokens.count - 1; i++) {
        if (!json_is_object(current)) {
            json_t *new_obj = json_object();
            json_object_set_new(current, tokens.tokens[i], new_obj);
            current = new_obj;
        } else {
            json_t *next = json_object_get(current, tokens.tokens[i]);

            if (!next) {
                next = json_object();
                json_object_set_new(current, tokens.tokens[i], next);
            }

            current = next;
        }
    }

    json_t *new_value = zval_to_json(value);

    if (new_value) {
        json_object_set_new(current, tokens.tokens[tokens.count - 1], new_value);
    }

    free_path_tokens(&tokens);

    return SUCCESS;
}

PHP_FUNCTION(json_get)
{
    char *json_str, *path;
    size_t json_len, path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &json_str, &json_len, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    json_t *root = get_cached_json(json_str, json_len);

    if (!root) {
        php_error_docref(NULL, E_WARNING, "Invalid JSON");
        RETURN_NULL();
    }

    path_tokens_t tokens = split_path_fast(path);
    json_t *current = root;

    for (size_t i = 0; i < tokens.count && current; i++) {
        if (!json_is_object(current)) {
            current = NULL;
            break;
        }

        current = json_object_get(current, tokens.tokens[i]);
    }

    if (!current) {
        free_path_tokens(&tokens);
        RETURN_NULL();
    }

    json_to_zval(return_value, current);
    free_path_tokens(&tokens);
}

PHP_FUNCTION(json_get_compiled)
{
    char *json_str;
    size_t json_len;
    zval *compiled_zv;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &json_str, &json_len, &compiled_zv) == FAILURE) {
        RETURN_FALSE;
    }

    zend_resource *res = Z_RES_P(compiled_zv);

    if (res->type != le_compiled_path) {
        php_error_docref(NULL, E_WARNING, "Invalid resource type");
        RETURN_NULL();
    }

    compiled_path_t *comp = (compiled_path_t*)res->ptr;

    if (!comp) {
        php_error_docref(NULL, E_WARNING, "Invalid compiled path");
        RETURN_NULL();
    }

    json_t *root = get_cached_json(json_str, json_len);

    if (!root) {
        RETURN_NULL();
    }

    json_t *current = root;

    for (int i = 0; i < comp->depth && current; i++) {
        if (!json_is_object(current)) {
            current = NULL;
            break;
        }

        current = json_object_get(current, comp->tokens[i]);
    }

    if (!current) {
        RETURN_NULL();
    }

    json_to_zval(return_value, current);
}

PHP_FUNCTION(json_set)
{
    char *json_str, *path;
    size_t json_len, path_len;
    zval *value;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssz", &json_str, &json_len, &path, &path_len, &value) == FAILURE) {
        RETURN_FALSE;
    }

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        php_error_docref(NULL, E_WARNING, "Invalid JSON: %s", error.text);
        RETURN_NULL();
    }

    apply_json_update(root, path, value);

    char *dump = json_dumps(root, JSON_COMPACT);
    RETVAL_STRING(dump);
    free(dump);

    json_decref(root);
}

PHP_FUNCTION(json_set_batch)
{
    char *json_str;
    size_t json_len;
    HashTable *updates;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sh", &json_str, &json_len, &updates) == FAILURE) {
        RETURN_FALSE;
    }

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        php_error_docref(NULL, E_WARNING, "Invalid JSON: %s", error.text);
        RETURN_NULL();
    }

    zval *entry;
    zend_string *path_key;

    ZEND_HASH_FOREACH_STR_KEY_VAL(updates, path_key, entry) {
        if (path_key) {
            apply_json_update(root, ZSTR_VAL(path_key), entry);
        }
    } ZEND_HASH_FOREACH_END();

    char *dump = json_dumps(root, JSON_COMPACT);
    RETVAL_STRING(dump);
    free(dump);

    json_decref(root);
}

PHP_FUNCTION(json_compile_path)
{
    char *path;
    size_t path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    compiled_path_t *comp = create_compiled_path(path);

    RETURN_RES(zend_register_resource(comp, le_compiled_path));
}

PHP_FUNCTION(json_free_compiled)
{
    UNUSED(return_value);

    zval *compiled_zv;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &compiled_zv) == FAILURE) {
        return;
    }

    zend_list_close(Z_RES_P(compiled_zv));
}

PHP_RSHUTDOWN_FUNCTION(jsonref)
{
    UNUSED(type);
    UNUSED(module_number);

    cleanup_json_cache();

    return SUCCESS;
}

const zend_function_entry jsonref_functions[] = {
    PHP_FE(json_get, arginfo_json_get)
    PHP_FE(json_get_compiled, arginfo_json_get_compiled)
    PHP_FE(json_set, arginfo_json_set)
    PHP_FE(json_set_batch, arginfo_json_set_batch)
    PHP_FE(json_compile_path, arginfo_json_compile_path)
    PHP_FE(json_free_compiled, arginfo_json_free_compiled)
    PHP_FE_END
};

zend_module_entry jsonref_module_entry = {
    STANDARD_MODULE_HEADER,
    "jsonref",
    jsonref_functions,
    PHP_MINIT(jsonref),
    NULL,
    NULL,
    PHP_RSHUTDOWN(jsonref),
    NULL,
    PHP_JSONREF_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_JSONREF
ZEND_GET_MODULE(jsonref)
#endif