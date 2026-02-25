#ifndef JSONREF_ARGINFO_H
#define JSONREF_ARGINFO_H

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_get, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_get_compiled, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
    ZEND_ARG_INFO(0, compiled_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_set, 0, 0, 3)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_set_batch, 0, 0, 2)
    ZEND_ARG_TYPE_INFO(0, json, IS_STRING, 0)
    ZEND_ARG_ARRAY_INFO(0, updates, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_compile_path, 0, 0, 1)
    ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_free_compiled, 0, 0, 1)
    ZEND_ARG_INFO(0, compiled_path)
ZEND_END_ARG_INFO()

#endif