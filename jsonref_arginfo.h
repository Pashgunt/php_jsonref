#ifndef PHP_JSONREF_ARGINFO_H
#define PHP_JSONREF_ARGINFO_H

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_get, 0, 0, 2)
    ZEND_ARG_INFO(0, json_str)
    ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_json_set, 0, 0, 3)
    ZEND_ARG_INFO(1, json_str)
    ZEND_ARG_INFO(0, path)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#endif
