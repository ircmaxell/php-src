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
#include "zend_string.h"

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
    zval ztype, zname, retval;
    zend_autoload_func *func_info;

    /* Verify autoload name before passing it to __autoload() */
    if (strspn(name->val, "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377\\") != name->len) {
        return NULL;
    }

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

    if (zend_hash_add_empty_element(stack, lname) == NULL) {
        // Recursion protection, add it early
        // as it will protect __autoload legacy behavior
        // as well
        return NULL;
    }

    if (zend_hash_num_elements(&EG(autoload.functions)) == 0) {
        if (type == ZEND_AUTOLOAD_CLASS) {
            zend_function *call = EG(autoload.legacy);
            if (!call) {
                call = (zend_function*) zend_hash_str_find_ptr(EG(function_table), ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME) - 1);
                EG(autoload.legacy) = call;
            }
            if (call) {
                zend_call_method_with_1_params(NULL, NULL, &call, ZEND_AUTOLOAD_FUNC_NAME, &retval, &zname);
                zend_hash_del(stack, lname);

                return zend_hash_find_ptr(symbol_table, lname);
            }
        }
        zend_hash_del(stack, lname);
        return NULL;
    }

    ZEND_HASH_FOREACH_PTR(&EG(autoload.functions), func_info)
        if (func_info->type & type) {
            func_info->fci.retval = &retval;
            zend_fcall_info_argn(&func_info->fci, 2, &zname, &ztype);
            zend_call_function(&func_info->fci, &func_info->fcc);
            zend_fcall_info_args_clear(&func_info->fci, 1);
            zend_exception_save();
            if (zend_hash_exists(symbol_table, lname)) {
                break;
            }
        }
    ZEND_HASH_FOREACH_END();

    zend_exception_restore();

    zend_hash_del(stack, lname);
    
    return zend_hash_find_ptr(symbol_table, lname);
}

int zend_autoload_register(zend_autoload_func* func, zend_bool prepend)
{

    if (zend_hash_next_index_insert_ptr(&EG(autoload.functions), func) == NULL) {
        return FAILURE;
    }

    if (prepend) {
        HT_MOVE_TAIL_TO_HEAD(&EG(autoload.functions));
    }
    return SUCCESS;
}

int zend_autoload_unregister(zend_autoload_func* func)
{
    zend_ulong h;

    zval *func_name = &func->fci.function_name;
    zval *current_name;
    zend_autoload_func *current;

    ZEND_HASH_FOREACH_NUM_KEY_PTR(&EG(autoload.functions), h, current)
        current_name = &current->fci.function_name;
        if (Z_TYPE_P(func_name) != Z_TYPE_P(current_name)) {
            continue;
        }
        switch (Z_TYPE_P(func_name)) {
            case IS_STRING:
                if (zend_string_equals(Z_STR_P(func_name), Z_STR_P(current_name))) {
                    // unset this one
                    zend_hash_index_del(&EG(autoload.functions), h);
                    return SUCCESS;
                }
            case IS_OBJECT:
                if (Z_OBJ_HANDLE_P(current_name) == Z_OBJ_HANDLE_P(func_name)) {
                    // unset this one
                    zend_hash_index_del(&EG(autoload.functions), h);
                    return SUCCESS;   
                }
        }
    ZEND_HASH_FOREACH_END();

    return FAILURE;
}

void zend_autoload_dtor(zval *pzv)
{
    zend_autoload_func *func = Z_PTR_P(pzv);
    efree(func);
}

static zend_bool zend_autoload_register_internal(INTERNAL_FUNCTION_PARAMETERS, long type)
{
    zend_autoload_func *func;
    zend_bool prepend = 0;
    int zpp_result;

    func = emalloc(sizeof(zend_autoload_func));

    func->type = type;

    if (type == ZEND_AUTOLOAD_ALL) {
        zpp_result = zend_parse_parameters(ZEND_NUM_ARGS(), "f|lb", &func->fci, &func->fcc, &func->type, &prepend);
    } else {
        zpp_result = zend_parse_parameters(ZEND_NUM_ARGS(), "f|b", &func->fci, &func->fcc, &prepend);
    }

    if (FAILURE == zpp_result) {
        efree(func);
        return 0;
    }
    
    if (zend_autoload_register(func, prepend) == FAILURE) {
        efree(func);
        return 0;
    }

    Z_ADDREF(func->fci.function_name);

    return 1;
}

ZEND_FUNCTION(autoload_register)
{
    RETURN_BOOL(zend_autoload_register_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_AUTOLOAD_ALL));
}

ZEND_FUNCTION(autoload_class_register) 
{
    RETURN_BOOL(zend_autoload_register_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_AUTOLOAD_CLASS));
}

ZEND_FUNCTION(autoload_function_register) 
{
    RETURN_BOOL(zend_autoload_register_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_AUTOLOAD_FUNCTION));
}

ZEND_FUNCTION(autoload_constant_register) 
{
    RETURN_BOOL(zend_autoload_register_internal(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_AUTOLOAD_CONSTANT));
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
