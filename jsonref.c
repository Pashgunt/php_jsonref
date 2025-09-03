#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_jsonref.h"
#include "jsonref.h"
#include <jansson.h>
#include <string.h>
#include "jsonref_arginfo.h"

static char **split_path(const char *path, size_t *count) {
    char *p = strdup(path);
    char *token;
    char **tokens = NULL;
    size_t n = 0;

    token = strtok(p, ".");

    while (token != NULL) {
        tokens = realloc(tokens, sizeof(char *) * (n + 1));
        tokens[n++] = strdup(token);
        token = strtok(NULL, ".");
    }

    free(p);
    *count = n;

    return tokens;
}

static void free_tokens(char **tokens, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(tokens[i]);
    }

    free(tokens);
}

PHP_FUNCTION(json_get)
{
    char *json_str, *path;
    size_t json_len, path_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &json_str, &json_len, &path, &path_len) == FAILURE) {
        RETURN_FALSE;
    }

    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);

    if (!root) {
        php_error_docref(NULL, E_WARNING, "Invalid JSON: %s", error.text);
        RETURN_NULL();
    }

    size_t path_count;
    char **tokens = split_path(path, &path_count);

    json_t *current = root;

    for (size_t i = 0; i < path_count; i++) {
        if (!json_is_object(current)) {
            current = NULL;
            break;
        }

        current = json_object_get(current, tokens[i]);

        if (!current) {
            break;
        }
    }

    free_tokens(tokens, path_count);

    if (!current) {
        json_decref(root);
        RETURN_NULL();
    }

    if (json_is_string(current)) {
        RETURN_STRING(json_string_value(current));
    } else if (json_is_integer(current)) {
        RETURN_LONG(json_integer_value(current));
    } else if (json_is_real(current)) {
        RETURN_DOUBLE(json_real_value(current));
    } else if (json_is_boolean(current)) {
        RETURN_BOOL(json_is_true(current));
    } else {
        char *dump = json_dumps(current, JSON_COMPACT);
        RETVAL_STRING(dump);
        free(dump);
    }

    json_decref(root);
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

    size_t path_count;
    char **tokens = split_path(path, &path_count);

    json_t *current = root;
    for (size_t i = 0; i < path_count - 1; i++) {
        json_t *next = json_object_get(current, tokens[i]);
        if (!next || !json_is_object(next)) {
            next = json_object();
            json_object_set_new(current, tokens[i], next);
        }
        current = next;
    }

    json_t *new_value = NULL;

    switch (Z_TYPE_P(value)) {
        case IS_STRING:
            new_value = json_string(Z_STRVAL_P(value));
            break;
        case IS_LONG:
            new_value = json_integer(Z_LVAL_P(value));
            break;
        case IS_DOUBLE:
            new_value = json_real(Z_DVAL_P(value));
            break;
        case IS_TRUE:
        case IS_FALSE:
            new_value = json_boolean(Z_TYPE_P(value) == IS_TRUE);
            break;
        default:
            php_error_docref(NULL, E_WARNING, "Unsupported value type");
            json_decref(root);
            free_tokens(tokens, path_count);
            RETURN_FALSE;
    }

    json_object_set_new(current, tokens[path_count - 1], new_value);

    char *dump = json_dumps(root, JSON_COMPACT);
    RETVAL_STRING(dump);
    free(dump);

    json_decref(root);
    free_tokens(tokens, path_count);
}

const zend_function_entry jsonref_functions[] = {
    PHP_FE(json_get, arginfo_json_get)
    PHP_FE(json_set, arginfo_json_set)
    PHP_FE_END
};

zend_module_entry jsonref_module_entry = {
    STANDARD_MODULE_HEADER,
    "jsonref",
    jsonref_functions,
    NULL, NULL, NULL, NULL, NULL,
    PHP_JSONREF_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_JSONREF
ZEND_GET_MODULE(jsonref)
#endif
