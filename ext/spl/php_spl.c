/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2014 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Marcus Boerger <helly@php.net>                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_main.h"
#include "ext/standard/info.h"
#include "php_spl.h"
#include "spl_functions.h"
#include "spl_engine.h"
#include "spl_array.h"
#include "spl_directory.h"
#include "spl_iterators.h"
#include "spl_exceptions.h"
#include "spl_observer.h"
#include "spl_dllist.h"
#include "spl_fixedarray.h"
#include "spl_heap.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "zend_autoload.h"
#include "ext/standard/php_rand.h"
#include "ext/standard/php_lcg.h"
#include "main/snprintf.h"

#ifdef COMPILE_DL_SPL
ZEND_GET_MODULE(spl)
#endif

ZEND_DECLARE_MODULE_GLOBALS(spl)

#define SPL_DEFAULT_FILE_EXTENSIONS ".inc,.php"

/* {{{ PHP_GINIT_FUNCTION
 */
static PHP_GINIT_FUNCTION(spl)
{
	spl_globals->autoload_extensions     = NULL;
	spl_globals->autoload_functions      = NULL;
	spl_globals->autoload_running        = 0;
}
/* }}} */

static zend_class_entry * spl_find_ce_by_name(zend_string *name, zend_bool autoload)
{
	zend_class_entry *ce;

	if (!autoload) {
		zend_string *lc_name = zend_string_alloc(name->len, 0);
		zend_str_tolower_copy(lc_name->val, name->val, name->len);

		ce = zend_hash_find_ptr(EG(class_table), lc_name);
		zend_string_free(lc_name);
	} else {
 		ce = zend_lookup_class(name);
 	}
 	if (ce == NULL) {
		php_error_docref(NULL, E_WARNING, "Class %s does not exist%s", name->val, autoload ? " and could not be loaded" : "");
		return NULL;
	}
	
	return ce;
}

/* {{{ proto array class_parents(object instance [, boolean autoload = true])
 Return an array containing the names of all parent classes */
PHP_FUNCTION(class_parents)
{
	zval *obj;
	zend_class_entry *parent_class, *ce;
	zend_bool autoload = 1;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
		php_error_docref(NULL, E_WARNING, "object or string expected");
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) == IS_STRING) {
		if (NULL == (ce = spl_find_ce_by_name(Z_STR_P(obj), autoload))) {
			RETURN_FALSE;
		}
	} else {
		ce = Z_OBJCE_P(obj);
	}
	
	array_init(return_value);
	parent_class = ce->parent;
	while (parent_class) {
		spl_add_class_name(return_value, parent_class, 0, 0);
		parent_class = parent_class->parent;
	}
}
/* }}} */

/* {{{ proto array class_implements(mixed what [, bool autoload ])
 Return all classes and interfaces implemented by SPL */
PHP_FUNCTION(class_implements)
{
	zval *obj;
	zend_bool autoload = 1;
	zend_class_entry *ce;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
		RETURN_FALSE;
	}
	if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
		php_error_docref(NULL, E_WARNING, "object or string expected");
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) == IS_STRING) {
		if (NULL == (ce = spl_find_ce_by_name(Z_STR_P(obj), autoload))) {
			RETURN_FALSE;
		}
	} else {
		ce = Z_OBJCE_P(obj);
	}
	
	array_init(return_value);
	spl_add_interfaces(return_value, ce, 1, ZEND_ACC_INTERFACE);
}
/* }}} */

/* {{{ proto array class_uses(mixed what [, bool autoload ])
 Return all traits used by a class. */
PHP_FUNCTION(class_uses)
{
	zval *obj;
	zend_bool autoload = 1;
	zend_class_entry *ce;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
		RETURN_FALSE;
	}
	if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
		php_error_docref(NULL, E_WARNING, "object or string expected");
		RETURN_FALSE;
	}
	
	if (Z_TYPE_P(obj) == IS_STRING) {
		if (NULL == (ce = spl_find_ce_by_name(Z_STR_P(obj), autoload))) {
			RETURN_FALSE;
		}
	} else {
		ce = Z_OBJCE_P(obj);
	}
	
	array_init(return_value);
	spl_add_traits(return_value, ce, 1, ZEND_ACC_TRAIT);
}
/* }}} */

#define SPL_ADD_CLASS(class_name, z_list, sub, allow, ce_flags) \
	spl_add_classes(spl_ce_ ## class_name, z_list, sub, allow, ce_flags)

#define SPL_LIST_CLASSES(z_list, sub, allow, ce_flags) \
	SPL_ADD_CLASS(AppendIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ArrayIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ArrayObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(BadFunctionCallException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(BadMethodCallException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(CachingIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(CallbackFilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(Countable, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(DirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(DomainException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(EmptyIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(FilesystemIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(FilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(GlobIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(InfiniteIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(InvalidArgumentException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(IteratorIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LengthException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LimitIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(LogicException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(MultipleIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(NoRewindIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OuterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OutOfBoundsException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OutOfRangeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(OverflowException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(ParentIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RangeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveArrayIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveCachingIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveCallbackFilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveDirectoryIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveFilterIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveIteratorIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveRegexIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RecursiveTreeIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RegexIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(RuntimeException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SeekableIterator, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplDoublyLinkedList, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplFileInfo, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplFileObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplFixedArray, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplHeap, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplMinHeap, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplMaxHeap, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplObjectStorage, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplObserver, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplPriorityQueue, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplQueue, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplStack, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplSubject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(SplTempFileObject, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(UnderflowException, z_list, sub, allow, ce_flags); \
	SPL_ADD_CLASS(UnexpectedValueException, z_list, sub, allow, ce_flags); \

/* {{{ proto array spl_classes()
 Return an array containing the names of all clsses and interfaces defined in SPL */
PHP_FUNCTION(spl_classes)
{
	array_init(return_value);
	
	SPL_LIST_CLASSES(return_value, 0, 0, 0)
}
/* }}} */

static int spl_autoload(zend_string *class_name, zend_string *lc_name, const char *ext, int ext_len) /* {{{ */
{
	char *class_file;
	int class_file_len;
	zval dummy;
	zend_file_handle file_handle;
	zend_op_array *new_op_array;
	zval result;
	int ret;

	class_file_len = (int)spprintf(&class_file, 0, "%s%.*s", lc_name->val, ext_len, ext);

#if DEFAULT_SLASH != '\\'
	{
		char *ptr = class_file;
		char *end = ptr + class_file_len;
		
		while ((ptr = memchr(ptr, '\\', (end - ptr))) != NULL) {
			*ptr = DEFAULT_SLASH;
		}
	}
#endif

	ret = php_stream_open_for_zend_ex(class_file, &file_handle, USE_PATH|STREAM_OPEN_FOR_INCLUDE);

	if (ret == SUCCESS) {
		zend_string *opened_path;
		if (!file_handle.opened_path) {
			file_handle.opened_path = estrndup(class_file, class_file_len);
		}
		opened_path = zend_string_init(file_handle.opened_path, strlen(file_handle.opened_path), 0);
		ZVAL_NULL(&dummy);
		if (zend_hash_add(&EG(included_files), opened_path, &dummy)) {
			new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE);
			zend_destroy_file_handle(&file_handle);
		} else {
			new_op_array = NULL;
			zend_file_handle_dtor(&file_handle);
		}
		zend_string_release(opened_path);
		if (new_op_array) {
			ZVAL_UNDEF(&result);
			zend_execute(new_op_array, &result);
	
			destroy_op_array(new_op_array);
			efree(new_op_array);
			if (!EG(exception)) {
				zval_ptr_dtor(&result);
			}

			efree(class_file);
			return zend_hash_exists(EG(class_table), lc_name);
		}
	}
	efree(class_file);
	return 0;
} /* }}} */

/* {{{ proto void spl_autoload(string class_name [, string file_extensions])
 Default implementation for __autoload() */
PHP_FUNCTION(spl_autoload)
{
	int found = 0, pos_len, pos1_len;
	char *pos, *pos1;
	zend_string *class_name, *lc_name, *file_exts = SPL_G(autoload_extensions);
	
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S", &class_name, &file_exts) == FAILURE) {
		RETURN_FALSE;
	}

	if (file_exts == NULL) { /* autoload_extensions is not initialized, set to defaults */
		pos = SPL_DEFAULT_FILE_EXTENSIONS;
		pos_len = sizeof(SPL_DEFAULT_FILE_EXTENSIONS) - 1;
	} else {
		pos = file_exts->val;
		pos_len = (int)file_exts->len;
	}

	lc_name = zend_string_alloc(class_name->len, 0);
	zend_str_tolower_copy(lc_name->val, class_name->val, class_name->len);
	while (pos && *pos && !EG(exception)) {
		pos1 = strchr(pos, ',');
		if (pos1) { 
			pos1_len = (int)(pos1 - pos);
		} else {
			pos1_len = pos_len;
		}
		if (spl_autoload(class_name, lc_name, pos, pos1_len)) {
			found = 1;
			break; /* loaded */
		}
		pos = pos1 ? pos1 + 1 : NULL;
		pos_len = pos1? pos_len - pos1_len - 1 : 0;
	}
	zend_string_free(lc_name);

	if (!found && !SPL_G(autoload_running)) {
		/* For internal errors, we generate E_ERROR, for direct calls an exception is thrown.
		 * The "scope" is determined by an opcode, if it is ZEND_FETCH_CLASS we know function was called indirectly by
		 * the Zend engine.
		 */
		zend_execute_data *ex = EX(prev_execute_data);

		while (ex && (!ex->func || !ZEND_USER_CODE(ex->func->type))) {
			ex = ex->prev_execute_data;
		}
		if (ex &&
		    ex->opline->opcode != ZEND_FETCH_CLASS &&
		    ex->opline->opcode != ZEND_NEW) {
			zend_throw_exception_ex(spl_ce_LogicException, 0, "Class %s could not be loaded", class_name->val);
		} else {
			php_error_docref(NULL, E_ERROR, "Class %s could not be loaded", class_name->val);
		}
	}
} /* }}} */

/* {{{ proto string spl_autoload_extensions([string file_extensions])
 Register and return default file extensions for spl_autoload */
PHP_FUNCTION(spl_autoload_extensions)
{
	zend_string *file_exts = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &file_exts) == FAILURE) {
		return;
	}
	if (file_exts) {
		if (SPL_G(autoload_extensions)) {
			zend_string_release(SPL_G(autoload_extensions));
		}
		SPL_G(autoload_extensions) = zend_string_copy(file_exts);
	}

	if (SPL_G(autoload_extensions) == NULL) {
		RETURN_STRINGL(SPL_DEFAULT_FILE_EXTENSIONS, sizeof(SPL_DEFAULT_FILE_EXTENSIONS) - 1);
	} else {
		zend_string_addref(SPL_G(autoload_extensions));
		RETURN_STR(SPL_G(autoload_extensions));
	}
} /* }}} */

/* {{{ proto void spl_autoload_call(string class_name)
 Try all registerd autoload function to load the requested class */
PHP_FUNCTION(spl_autoload_call)
{
	zval *class_name;
	zend_string *lc_name;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &class_name) == FAILURE || Z_TYPE_P(class_name) != IS_STRING) {
		return;
	}

    if (zend_hash_num_elements(&EG(autoload.functions)) == 0) {
        /* do not use or overwrite &EG(autoload_func) here */
        zend_call_method_with_1_params(NULL, NULL, NULL, "spl_autoload", NULL, class_name);
    } else {
        lc_name = zend_string_alloc(Z_STRLEN_P(class_name), 0);
        zend_str_tolower_copy(lc_name->val, Z_STRVAL_P(class_name), Z_STRLEN_P(class_name));

        zend_autoload_call(Z_STR_P(class_name), lc_name, ZEND_AUTOLOAD_CLASS);
        zend_string_free(lc_name);
    }
} /* }}} */

/* {{{ proto bool spl_autoload_register([mixed autoload_function = "spl_autoload" [, throw = true [, prepend]]])
 Register given function as __autoload() implementation */
PHP_FUNCTION(spl_autoload_register)
{
    zval *zcallable = NULL;
    zend_autoload_func *func;
    int success = FAILURE;
    zend_bool do_throw = 0, prepend = 0;

    func = emalloc(sizeof(zend_autoload_func));
    func->type = ZEND_AUTOLOAD_CLASS;

    if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "f|bb", &func->fci, &func->fcc, &do_throw, &prepend) == SUCCESS) {
        success = zend_autoload_register(func, prepend);
        if (FAILURE == success) {
            efree(func);
        }
        RETURN_BOOL(success == SUCCESS);
    }

    efree(func);

    if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|zbb", &zcallable, &do_throw, &prepend) == FAILURE) {
        return;
    }

    if (ZEND_NUM_ARGS()) {
        /* Legacy error handling */
        zend_fcall_info_cache fcc;
        zend_string *func_name;
        char *error = NULL;
        zend_object *obj_ptr;
        if (!zend_is_callable_ex(zcallable, NULL, IS_CALLABLE_STRICT, &func_name, &fcc, &error)) {
            obj_ptr = fcc.object;
            if (Z_TYPE_P(zcallable) == IS_ARRAY) {
                if (!obj_ptr && fcc.function_handler && !(fcc.function_handler->common.fn_flags & ZEND_ACC_STATIC)) {
                    if (do_throw) {
                        zend_throw_exception_ex(spl_ce_LogicException, 0, "Passed array specifies a non static method but no object (%s)", error);
                    }
                    if (error) {
                        efree(error);
                    }
                    zend_string_release(func_name);
                    RETURN_FALSE;
                } else if (do_throw) {
                    zend_throw_exception_ex(spl_ce_LogicException, 0, "Passed array does not specify %s %smethod (%s)", fcc.function_handler ? "a callable" : "an existing", !obj_ptr ? "static " : "", error);
                }
                if (error) {
                    efree(error);
                }
                zend_string_release(func_name);
                RETURN_FALSE;
            } else if (Z_TYPE_P(zcallable) == IS_STRING) {
                if (do_throw) {
                    zend_throw_exception_ex(spl_ce_LogicException, 0, "Function '%s' not %s (%s)", func_name->val, fcc.function_handler ? "callable" : "found", error);
                }
                if (error) {
                    efree(error);
                }
                zend_string_release(func_name);
                RETURN_FALSE;
            } else {
                if (do_throw) {
                    zend_throw_exception_ex(spl_ce_LogicException, 0, "Illegal value passed (%s)", error);
                }
                if (error) {
                    efree(error);
                }
                zend_string_release(func_name);
                RETURN_FALSE;
            }
        } else if (fcc.function_handler->type == ZEND_INTERNAL_FUNCTION &&
                   fcc.function_handler->internal_function.handler == zif_spl_autoload_call) {
            if (do_throw) {
                zend_throw_exception_ex(spl_ce_LogicException, 0, "Function spl_autoload_call() cannot be registered");
            }
            if (error) {
                efree(error);
            }
            zend_string_release(func_name);
            RETURN_FALSE;
        }
        ZEND_ASSERT(1==0);
    } else {
        /* TODO: add support for empty registering spl_autoload */
    }

    
	RETURN_FALSE;
} /* }}} */

/* {{{ proto bool spl_autoload_unregister(mixed autoload_function)
 Unregister given function as __autoload() implementation */
PHP_FUNCTION(spl_autoload_unregister)
{
	zend_string *func_name = NULL;
	char *error = NULL;
	zval *zcallable;
	int success = FAILURE;
    zend_fcall_info_cache fcc;

    zend_autoload_func *func;

    func = emalloc(sizeof(zend_autoload_func));

    if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "f", &func->fci, &func->fcc) == SUCCESS) {
        success = zend_autoload_unregister(func);
        efree(func);
        RETURN_BOOL(success);
    }
    efree(func);

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &zcallable) == FAILURE) {
		return;
	}

	if (!zend_is_callable_ex(zcallable, NULL, IS_CALLABLE_CHECK_SYNTAX_ONLY, &func_name, &fcc, &error)) {
		zend_throw_exception_ex(spl_ce_LogicException, 0, "Unable to unregister invalid function (%s)", error);
		if (error) {
			efree(error);
		}
		if (func_name) {
			zend_string_release(func_name);
		}
		RETURN_FALSE;
	}

    /* Should not happen, indicates failure in "f" ZPP mode */
    RETURN_FALSE;
} /* }}} */

/* {{{ proto false|array spl_autoload_functions()
 Return all registered __autoload() functionns */
PHP_FUNCTION(spl_autoload_functions)
{
    zend_autoload_func *func_info;
    zval *callable;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}
	
    if (zend_hash_num_elements(&EG(autoload.functions)) == 0) {
        if (NULL != zend_hash_str_find_ptr(EG(function_table), ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME) - 1)) {
            array_init(return_value);
            add_next_index_stringl(return_value, ZEND_AUTOLOAD_FUNC_NAME, sizeof(ZEND_AUTOLOAD_FUNC_NAME) - 1);
            return;
        }
        RETURN_FALSE;
    }
    array_init(return_value);
    ZEND_HASH_FOREACH_PTR(&EG(autoload.functions), func_info)
        add_next_index_zval(return_value, &func_info->fci.function_name);
    ZEND_HASH_FOREACH_END();
} /* }}} */

/* {{{ proto string spl_object_hash(object obj)
 Return hash id for given object */
PHP_FUNCTION(spl_object_hash)
{
	zval *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &obj) == FAILURE) {
		return;
	}
	
	RETURN_NEW_STR(php_spl_object_hash(obj));
}
/* }}} */

PHPAPI zend_string *php_spl_object_hash(zval *obj) /* {{{*/
{
	intptr_t hash_handle, hash_handlers;

	if (!SPL_G(hash_mask_init)) {
		if (!BG(mt_rand_is_seeded)) {
			php_mt_srand((uint32_t)GENERATE_SEED());
		}

		SPL_G(hash_mask_handle)   = (intptr_t)(php_mt_rand() >> 1);
		SPL_G(hash_mask_handlers) = (intptr_t)(php_mt_rand() >> 1);
		SPL_G(hash_mask_init) = 1;
	}

	hash_handle   = SPL_G(hash_mask_handle)^(intptr_t)Z_OBJ_HANDLE_P(obj);
	hash_handlers = SPL_G(hash_mask_handlers)^(intptr_t)Z_OBJ_HT_P(obj);

	return strpprintf(32, "%016lx%016lx", hash_handle, hash_handlers);
}
/* }}} */

int spl_build_class_list_string(zval *entry, char **list) /* {{{ */
{
	char *res;
	
	spprintf(&res, 0, "%s, %s", *list, Z_STRVAL_P(entry));
	efree(*list);
	*list = res;
	return ZEND_HASH_APPLY_KEEP;
} /* }}} */

/* {{{ PHP_MINFO(spl)
 */
PHP_MINFO_FUNCTION(spl)
{
	zval list;
	char *strg;

	php_info_print_table_start();
	php_info_print_table_header(2, "SPL support",        "enabled");

	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, 1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg);
	zval_dtor(&list);
	php_info_print_table_row(2, "Interfaces", strg + 2);
	efree(strg);

	array_init(&list);
	SPL_LIST_CLASSES(&list, 0, -1, ZEND_ACC_INTERFACE)
	strg = estrdup("");
	zend_hash_apply_with_argument(Z_ARRVAL_P(&list), (apply_func_arg_t)spl_build_class_list_string, &strg);
	zval_dtor(&list);
	php_info_print_table_row(2, "Classes", strg + 2);
	efree(strg);

	php_info_print_table_end();
}
/* }}} */

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_iterator_to_array, 0, 0, 1)
	ZEND_ARG_OBJ_INFO(0, iterator, Traversable, 0)
	ZEND_ARG_INFO(0, use_keys)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO(arginfo_iterator, 0)
	ZEND_ARG_OBJ_INFO(0, iterator, Traversable, 0)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_iterator_apply, 0, 0, 2)
	ZEND_ARG_OBJ_INFO(0, iterator, Traversable, 0)
	ZEND_ARG_INFO(0, function)
	ZEND_ARG_ARRAY_INFO(0, args, 1)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_parents, 0, 0, 1)
	ZEND_ARG_INFO(0, instance)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_implements, 0, 0, 1)
	ZEND_ARG_INFO(0, what)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_uses, 0, 0, 1)
	ZEND_ARG_INFO(0, what)
	ZEND_ARG_INFO(0, autoload)
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_spl_classes, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_spl_autoload_functions, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_autoload, 0, 0, 1)
	ZEND_ARG_INFO(0, class_name)
	ZEND_ARG_INFO(0, file_extensions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_autoload_extensions, 0, 0, 0)
	ZEND_ARG_INFO(0, file_extensions)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_autoload_call, 0, 0, 1)
	ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_autoload_register, 0, 0, 0)
	ZEND_ARG_INFO(0, autoload_function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_autoload_unregister, 0, 0, 1)
	ZEND_ARG_INFO(0, autoload_function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_spl_object_hash, 0, 0, 1)
	ZEND_ARG_INFO(0, obj)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ spl_functions
 */
const zend_function_entry spl_functions[] = {
	PHP_FE(spl_classes,             arginfo_spl_classes)
	PHP_FE(spl_autoload,            arginfo_spl_autoload)
	PHP_FE(spl_autoload_extensions, arginfo_spl_autoload_extensions)
	PHP_FE(spl_autoload_register,   arginfo_spl_autoload_register)
	PHP_FE(spl_autoload_unregister, arginfo_spl_autoload_unregister)
	PHP_FE(spl_autoload_functions,  arginfo_spl_autoload_functions)
	PHP_FE(spl_autoload_call,       arginfo_spl_autoload_call)
	PHP_FE(class_parents,           arginfo_class_parents)
	PHP_FE(class_implements,        arginfo_class_implements)
	PHP_FE(class_uses,              arginfo_class_uses)
	PHP_FE(spl_object_hash,         arginfo_spl_object_hash)
#ifdef SPL_ITERATORS_H
	PHP_FE(iterator_to_array,       arginfo_iterator_to_array)
	PHP_FE(iterator_count,          arginfo_iterator)
	PHP_FE(iterator_apply,          arginfo_iterator_apply)
#endif /* SPL_ITERATORS_H */
	PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION(spl)
 */
PHP_MINIT_FUNCTION(spl)
{
	PHP_MINIT(spl_exceptions)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_iterators)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_array)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_directory)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_dllist)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_heap)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_fixedarray)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(spl_observer)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

PHP_RINIT_FUNCTION(spl) /* {{{ */
{
	SPL_G(autoload_extensions) = NULL;
	SPL_G(autoload_functions) = NULL;
	SPL_G(hash_mask_init) = 0;
	return SUCCESS;
} /* }}} */

PHP_RSHUTDOWN_FUNCTION(spl) /* {{{ */
{
	if (SPL_G(autoload_extensions)) {
		zend_string_release(SPL_G(autoload_extensions));
		SPL_G(autoload_extensions) = NULL;
	}
	if (SPL_G(autoload_functions)) {
		zend_hash_destroy(SPL_G(autoload_functions));
		FREE_HASHTABLE(SPL_G(autoload_functions));
		SPL_G(autoload_functions) = NULL;
	}
	if (SPL_G(hash_mask_init)) {
		SPL_G(hash_mask_init) = 0;
	}
	return SUCCESS;
} /* }}} */

/* {{{ spl_module_entry
 */
zend_module_entry spl_module_entry = {
	STANDARD_MODULE_HEADER,
	"SPL",
	spl_functions,
	PHP_MINIT(spl),
	NULL,
	PHP_RINIT(spl),
	PHP_RSHUTDOWN(spl),
	PHP_MINFO(spl),
	"0.2",
	PHP_MODULE_GLOBALS(spl),
	PHP_GINIT(spl),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
