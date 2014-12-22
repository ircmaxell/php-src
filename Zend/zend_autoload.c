/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2014 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Anthony Ferrara <ircmaxell@php.net>                         |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "zend.h"
#include "zend_API.h"
#include "zend_autoload.h"
#include "zend_hash.h"
#include "zend_types.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"

#define HT_MOVE_TAIL_TO_HEAD(ht)                            \
    do {                                                        \
        Bucket tmp = (ht)->arData[(ht)->nNumUsed-1];                \
        memmove((ht)->arData + 1, (ht)->arData,                 \
            sizeof(Bucket) * ((ht)->nNumUsed - 1));             \
        (ht)->arData[0] = tmp;                                  \
        zend_hash_rehash(ht);                                   \
    } while (0)


void* zend_autoload_call(zend_string *name, zend_string *lname, long type)
{
    HashTable *symbol_table, *stack;
    zval dummy, ztype, zname, retval;
    zend_autoload_func *func_info;

    ZVAL_UNDEF(&dummy);
    ZVAL_LONG(&ztype, type);
    ZVAL_STR(&zname, name);

    switch (type) {
        case ZEND_AUTOLOAD_CLASS:
            symbol_table = EG(class_table);
            stack = &EG(autoload.stack.class);
            break;
        case ZEND_AUTOLOAD_FUNCTION:
            symbol_table = EG(function_table);
            stack = &EG(autoload.stack.function);
            break;
        case ZEND_AUTOLOAD_CONSTANT:
            symbol_table = EG(zend_constants);
            stack = &EG(autoload.stack.constant);
            break;
        default:
            return NULL;
    }

    if (zend_hash_num_elements(&EG(autoload.functions)) == 0) {
        if (type == ZEND_AUTOLOAD_CLASS) {
            zend_function *call = EG(autoload.legacy);
            if (!call) {
                call = (zend_function*) zend_hash_str_find_ptr(EG(function_table), ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME));
                EG(autoload.legacy) = call;
            }
            if (call) {
                zend_call_method_with_1_params(NULL, NULL, &call, ZEND_AUTOLOAD_FUNC_NAME, &retval, &zname);
                return zend_hash_find_ptr(symbol_table, lname);
            }
        }
        return NULL;
    }

    if (zend_hash_add(stack, lname, &dummy) == NULL) {
        // Recursion protection
        return NULL;
    }

    ZEND_HASH_FOREACH_PTR(&EG(autoload.functions), func_info)
        if (func_info->type & type) {
            func_info->fci.retval = &retval;
            zend_fcall_info_argn(&func_info->fci, 2, &zname, &ztype);
            zend_call_function(&func_info->fci, &func_info->fcc);
            zend_exception_save();
            if (zend_hash_exists(symbol_table, lname)) {
                break;
            }
        }
    ZEND_HASH_FOREACH_END();

    zend_fcall_info_args_clear(&func_info->fci, 1);
    zend_exception_restore();

    zend_hash_del(stack, lname);

    return zend_hash_find_ptr(symbol_table, lname);
}

static zend_string* zend_autoload_get_key(zend_fcall_info *fci)
{
    switch (Z_TYPE(fci->function_name)) {
        case IS_STRING:
            return Z_STR(fci->function_name);
        case IS_OBJECT: {
            char handle[16];
            sprintf(handle, "%lu", (unsigned long) Z_OBJ_HANDLE(fci->function_name));
            return zend_string_init(handle, sizeof(uint32_t), 0);
        }
    }
    return NULL;
}


int zend_autoload_register(zend_autoload_func* func, zend_bool prepend)
{
    zend_string *key;

    key = zend_autoload_get_key(&func->fci);
    if (key == NULL) {
        zend_error_noreturn(E_ERROR, "Unknown function type provided");
    }

    if (zend_hash_add_ptr(&EG(autoload.functions), key, func) == NULL) {
        return FAILURE;
    }

    if (prepend) {
        HT_MOVE_TAIL_TO_HEAD(&EG(autoload.functions));
    }
    return SUCCESS;
}

int zend_autoload_unregister(zend_autoload_func* func)
{
    zend_string *key;

    key = zend_autoload_get_key(&func->fci);

    zend_hash_del(&EG(autoload.functions), key);

    return SUCCESS;
}

ZEND_FUNCTION(autoload_register)
{
    zend_autoload_func *func;
    zend_bool prepend = 0;

    func = emalloc(sizeof(zend_autoload_func));

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "f|lb", &func->fci, &func->fcc, &func->type, &prepend) == FAILURE) {
        efree(func);
        return;
    }

    if (!func->type) {
        func->type = ZEND_AUTOLOAD_ALL;
    }

    if (zend_autoload_register(func, prepend) == FAILURE) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

ZEND_FUNCTION(autoload_unregister)
{
    zend_autoload_func *func;

    func = emalloc(sizeof(zend_autoload_func));

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "f", &func->fci, &func->fcc) == FAILURE) {
        efree(func);
        return;
    }

    if (zend_autoload_unregister(func) == FAILURE) {
        efree(func);
        RETURN_FALSE;
    }
    efree(func);
    RETURN_TRUE;
}