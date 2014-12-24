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
        if (!((ht)->u.flags & HASH_FLAG_PACKED)) {              \
            zend_hash_rehash(ht);                               \
        } else {                                                \
            zend_autoload_reindex(ht);                          \
        }                                                       \
    } while (0)

static void zend_autoload_reindex(HashTable *ht)
{
    size_t i;
    ZEND_ASSERT(ht->u.flags & HASH_FLAG_PACKED);
    for (i = 0; i < ht->nNumUsed; i++) {
        ht->arData[i].h = i;
    }
}


static zend_always_inline int zend_autoload_callback_equals(zend_autoload_func* func, zend_autoload_func* current) 
{
    zval *func_name = &func->fci.function_name;
    zval *current_name = &current->fci.function_name;

    if (Z_TYPE_P(func_name) != Z_TYPE_P(current_name)) {
        return FAILURE;
    }
    switch (Z_TYPE_P(func_name)) {
        case IS_STRING:
            if (zend_string_equals(Z_STR_P(func_name), Z_STR_P(current_name))) {
                return SUCCESS;
            }
        case IS_OBJECT:
            if (Z_OBJ_HANDLE_P(current_name) == Z_OBJ_HANDLE_P(func_name)) {
                return SUCCESS;   
            }
    }
    return FAILURE;
}



void* zend_autoload_call(zend_string *name, zend_string *lname, long type)
{
    void *value = NULL;
    HashTable *symbol_table, *stack;
    zval zname, retval;
    zend_autoload_func *func_info;
    zend_bool dtor_name = 0, dtor_lname = 0;

    if (UNEXPECTED(name->val[0] == '\\')) {
        /* need to remove leading \ */
        name = zend_string_init(name->val + 1, name->len - 1, 0);
        dtor_name = 1;
    }
    if (UNEXPECTED(lname->val[0] == '\\')) {
        /* need to remove leading \ 
         * This is a separate check, since some places will strip
         * the leading slash from the lname already as it's used as a key
         */
        lname = zend_string_init(lname->val + 1, lname->len - 1, 0);
        dtor_lname = 1;
    }

    /* Verify autoload name before passing it to __autoload() */
    if (strspn(name->val, "0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377\\") != name->len) {
        goto return_null;
    }

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
            goto return_null;
    }

    if (zend_hash_add_empty_element(stack, lname) == NULL) {
        // Recursion protection, add it early
        // as it will protect __autoload legacy behavior
        // as well
        goto return_null;
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
                goto return_lookup;
            }
        }
        zend_hash_del(stack, lname);
        goto return_null;
    }

    ZEND_HASH_FOREACH_PTR(&EG(autoload.functions), func_info)
        if (func_info->type & type) {
            func_info->fci.retval = &retval;
            zend_fcall_info_argn(&func_info->fci, 1, &zname);
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
    
return_lookup:
    value = zend_hash_find_ptr(symbol_table, lname);
    if (dtor_name) {
        /* release the string, as an autoloader may have aquired a reference to it */
        zend_string_release(name);
    }
    if (dtor_lname) {
        zend_string_free(lname);
    }
    return value;

return_null:
    if (dtor_name) {
        /* release the string, as an autoloader may have aquired a reference to it */
        zend_string_release(name);
    }
    if (dtor_lname) {
        zend_string_free(lname);
    }
    return NULL;
}

int zend_autoload_register(zend_autoload_func* func, long flags)
{
    zend_autoload_func *current;

    ZEND_HASH_FOREACH_PTR(&EG(autoload.functions), current)
        if (SUCCESS == zend_autoload_callback_equals(func, current)) {
            if (current->type == func->type) {
                /* already registered!!! */
                zval_ptr_dtor(&func->fci.function_name);
                efree(func);
                return SUCCESS;
            }
        }
    ZEND_HASH_FOREACH_END();

    Z_TRY_ADDREF(func->fci.function_name);
    if (zend_hash_next_index_insert_ptr(&EG(autoload.functions), func) == NULL) {
        Z_TRY_DELREF(func->fci.function_name);
        return FAILURE;
    }

    if (func->type & ZEND_AUTOLOAD_CLASS) {
        EG(autoload.class_loader_count)++;
    }

    if (flags & ZEND_AUTOLOAD_FLAG_PREPEND) {
        HT_MOVE_TAIL_TO_HEAD(&EG(autoload.functions));
    }

    return SUCCESS;
}

int zend_autoload_unregister(zend_autoload_func* func)
{
    zend_ulong h;
    zend_autoload_func *current;
    int retval = FAILURE;

    ZEND_HASH_FOREACH_NUM_KEY_PTR(&EG(autoload.functions), h, current)
        if (SUCCESS == zend_autoload_callback_equals(func, current)) {
            if (current->type & ZEND_AUTOLOAD_CLASS) {
                EG(autoload.class_loader_count)--;
            }
            zend_hash_index_del(&EG(autoload.functions), h);
            retval = SUCCESS;
        }
    ZEND_HASH_FOREACH_END();
    return retval;
}

int zend_autoload_unregister_all(long type)
{
    zend_ulong h;
    zend_autoload_func *current;

    ZEND_HASH_FOREACH_NUM_KEY_PTR(&EG(autoload.functions), h, current)
        if (current->type & type) {
            if (current->type & ZEND_AUTOLOAD_CLASS) {
                EG(autoload.class_loader_count)--;
            }
            zend_hash_index_del(&EG(autoload.functions), h);
        }
    ZEND_HASH_FOREACH_END();
    return SUCCESS;
}

void zend_autoload_dtor(zval *pzv)
{
    zend_autoload_func *func = Z_PTR_P(pzv);
    zval_ptr_dtor(&func->fci.function_name);
    if (func->type & ZEND_AUTOLOAD_CLASS) {
        EG(autoload.class_loader_count)--;
    }
    efree(func);
}

ZEND_FUNCTION(autoload_register)
{
    zend_autoload_func *func;
    long flags = 0;

    func = emalloc(sizeof(zend_autoload_func));

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "lf|l", &func->type, &func->fci, &func->fcc, &flags)) {
        efree(func);
        return;
    }

    switch (func->type) {
        case ZEND_AUTOLOAD_CLASS:
        case ZEND_AUTOLOAD_FUNCTION:
        case ZEND_AUTOLOAD_CONSTANT:
            break;
        default:
            zend_error(E_WARNING, "Provided autoloader type is invalid: %d", func->type);
            RETURN_FALSE;
    }

    if (zend_autoload_register(func, flags) == FAILURE) {
        efree(func);
        RETURN_FALSE;
    }

    Z_TRY_ADDREF(func->fci.function_name);

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

ZEND_FUNCTION(autoload_unregister_all)
{
    long type = ~0;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|l", &type) == FAILURE) {
        return;
    }

    RETURN_BOOL(SUCCESS == zend_autoload_unregister_all(type));
}
