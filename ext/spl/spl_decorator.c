/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Antony Dovgal <tony@daylessday.org>                          |
  |         Etienne Kneuss <colder@php.net>                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "zend_exceptions.h"
#include "zend_object_handlers.h"
#include "php_spl.h"
#include "spl_functions.h"
#include "spl_decorator.h"
#include "spl_engine.h"
#include "spl_exceptions.h"

PHPAPI zend_class_entry *spl_ce_SplDecorator;
zend_object_handlers spl_handler_SplDecorator;

typedef struct _spl_decorator_object {
	zend_object       std;
	zval              *parent;
} spl_decorator_object;

static void spl_decorator_object_free_storage(void *object TSRMLS_DC) /* {{{ */
{
	spl_decorator_object *intern = (spl_decorator_object *)object;

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	FREE_ZVAL(intern->parent);

	efree(object);
}
/* }}} */

static zend_object_value spl_decorator_new(zend_class_entry *class_type TSRMLS_DC) /* {{{ */
{
	spl_decorator_object *decorator;
	zend_object_value object;

	decorator = emalloc(sizeof(spl_decorator_object));
	memset(decorator, 0, sizeof(spl_decorator_object));

	zend_object_std_init(&decorator->std, class_type TSRMLS_CC);

	object.handle = zend_objects_store_put(decorator, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) spl_decorator_object_free_storage, NULL TSRMLS_CC);
	object.handlers = &spl_handler_SplDecorator;

	ALLOC_INIT_ZVAL(decorator->parent);
	return object;
}
/* }}} */


/* {{{ proto void SplDecorator::__construct(object wrapped)
*/
SPL_METHOD(SplDecorator, __construct)
{
	zval *object = getThis();
	zval *parent;
	zend_class_entry *ce;
	spl_decorator_object *intern = (spl_decorator_object*)zend_object_store_get_object(object TSRMLS_CC);

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o", &parent)) {
		return;
	}

	ZVAL_ZVAL(intern->parent, parent, 1, 1);

	/* Add one to the size for the parent interface */
	ce = Z_OBJCE_P(object);
	ce->interfaces = safe_erealloc(ce->interfaces, ce->num_interfaces, sizeof(zend_class_entry), 0);
	ce->interfaces[ce->num_interfaces] = Z_OBJCE_P(parent);
	ce->num_interfaces++;
	
	/* TODO: Update Handlers */
	
}
/* }}} */

SPL_METHOD(SplDecorator, __call)
{
	zval *object = getThis(), *array = NULL, *retval_ptr = NULL, *method = NULL;
	zval ***params = NULL;
	spl_decorator_object *intern = (spl_decorator_object*)zend_object_store_get_object(object TSRMLS_CC);
	int n_params = 0;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za", &method, &array)) {
		return;
	}

	n_params = zend_hash_num_elements(Z_ARRVAL_P(array));

	if (n_params > 0) {
		long i = 0;
		zval **element;
		params = safe_emalloc(n_params, sizeof(zval**), 0);
		for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(array));
			zend_hash_get_current_data(Z_ARRVAL_P(array), (void **) &element) == SUCCESS;
			zend_hash_move_forward(Z_ARRVAL_P(array))
		) {
			params[i] = element;
			i++;
		}
			
	}

	if (SUCCESS == call_user_function_ex(EG(function_table), &(intern->parent), method, &retval_ptr, n_params, params, 0, NULL TSRMLS_CC)) {
		if (retval_ptr) {
			COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to call %s()", Z_STRVAL_P(method));
	}
	if (n_params) {
		efree(params);
	}
	
}

SPL_METHOD(SplDecorator, getDecoratedObject) {
	zval *object = getThis();
	spl_decorator_object *intern = (spl_decorator_object*)zend_object_store_get_object(object TSRMLS_CC);
	RETURN_ZVAL(intern->parent, 1, 0);
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_spldecorator_construct, 0, 0, 1)
	ZEND_ARG_INFO(0, obj)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_spldecorator_call, 0, 0, 2)
	ZEND_ARG_INFO(0, method)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()
ZEND_BEGIN_ARG_INFO_EX(arginfo_spldecorator_getdecoratedobject, 0, 0, 0)
ZEND_END_ARG_INFO()

static zend_function_entry spl_funcs_SplDecorator[] = { /* {{{ */
	SPL_ME(SplDecorator, __construct, arginfo_spldecorator_construct,ZEND_ACC_PUBLIC)
	SPL_ME(SplDecorator, __call, arginfo_spldecorator_call, ZEND_ACC_PUBLIC)
	SPL_ME(SplDecorator, getDecoratedObject, arginfo_spldecorator_getdecoratedobject, ZEND_ACC_PUBLIC)
	PHP_FE_END
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(spl_decorator)
{
	REGISTER_SPL_STD_CLASS_EX(SplDecorator, spl_decorator_new, spl_funcs_SplDecorator);
	memcpy(&spl_handler_SplDecorator, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

	return SUCCESS;
}
/* }}} */




/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
