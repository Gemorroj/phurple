/**
 * Copyright (c) 2007-2011, Anatol Belski <ab@php.net>
 *
 * This file is part of Phurple.
 *
 * Phurple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Phurple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Phurple.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>

#include "php_phurple.h"

#include <glib.h>

#include <string.h>
#include <ctype.h>

#include <purple.h>

#if PHURPLE_INTERNAL_DEBUG
extern void phurple_dump_zval(zval *var);
#endif

void
php_buddygroup_obj_destroy(void *obj TSRMLS_DC)
{
	struct ze_buddygroup_obj *zgo = (struct ze_buddygroup_obj *)obj;

	zend_object_std_dtor(&zgo->zo TSRMLS_CC);

	/*if (zgo->pbuddygroup) {
		purple_group_destroy(zgo->pbuddygroup);
	}*/

	efree(zgo);
}

zend_object_value
php_buddygroup_obj_init(zend_class_entry *ce TSRMLS_DC)
{
	zend_object_value ret;
	struct ze_buddygroup_obj *zgo;
#if PHP_MAJOR_VERSION > 5 || PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION < 4
	zval *tmp;
#endif

	zgo = (struct ze_buddygroup_obj *) emalloc(sizeof(struct ze_buddygroup_obj));
	memset(&zgo->zo, 0, sizeof(zend_object));

	zend_object_std_init(&zgo->zo, ce TSRMLS_CC);
#if PHP_MAJOR_VERSION > 5 || PHP_MAJOR_VERSION== 5 && PHP_MINOR_VERSION < 4
	zend_hash_copy(zgo->zo.properties, &ce->default_properties, (copy_ctor_func_t) zval_add_ref,
					(void *) &tmp, sizeof(zval *));
#else
	object_properties_init(&zgo->zo, ce);
#endif

	zgo->pbuddygroup = NULL;

	ret.handle = zend_objects_store_put(zgo, NULL,
								(zend_objects_free_object_storage_t) php_buddygroup_obj_destroy,
								NULL TSRMLS_CC);

	ret.handlers = &default_phurple_obj_handlers;

	return ret;
}

/*
**
**
** Phurple BuddyGroup methods
**
*/

/* {{{ proto object PhurpleBuddyGroup::__construct(void)
	constructor*/
PHP_METHOD(PhurpleBuddyGroup, __construct)
{
	char *name;
	int name_len;
	struct ze_buddygroup_obj *zgo;

	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		RETURN_NULL();
	}

	zgo = (struct ze_buddygroup_obj *) zend_object_store_get_object(getThis() TSRMLS_CC);
	zgo->pbuddygroup = purple_find_group(name);

	if(zgo->pbuddygroup) {
		zgo->pbuddygroup = purple_group_new(name);
	}

	if (NULL == zgo->pbuddygroup) {
		zend_throw_exception_ex(PhurpleException_ce, "Failed to create group", 0 TSRMLS_CC);
		return;
	}
}
/* }}} */


/* {{{ proto array PhurpleBuddyGroup::getAccounts(void)
	gets all the accounts related to the group */
PHP_METHOD(PhurpleBuddyGroup, getAccounts)
{
	struct ze_buddygroup_obj *zgo;
	GSList *iter;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zgo = (struct ze_buddygroup_obj *) zend_object_store_get_object(getThis() TSRMLS_CC);
	
	iter = purple_group_get_accounts(zgo->pbuddygroup);
		
	if(iter && g_slist_length(iter)) {
		array_init(return_value);
		
		for (; iter; iter = iter->next) {
			PurpleAccount *paccount = iter->data;
			
			if (paccount) {
				zval *account;
				struct ze_account_obj *zao;

				PHURPLE_MK_OBJ(account, PhurpleAccount_ce);
				zao = (struct ze_account_obj *) zend_object_store_get_object(account TSRMLS_CC);
				zao->paccount = paccount;

				add_next_index_zval(return_value, account);
			}
		}

		return;
	}
	
	RETURN_NULL();
}
/* }}} */


/* {{{ proto int PhurpleBuddyGroup::getSize(void)
	gets the count of the buddies in the group */
PHP_METHOD(PhurpleBuddyGroup, getSize)
{
	struct ze_buddygroup_obj *zgo;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zgo = (struct ze_buddygroup_obj *) zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_LONG(purple_blist_get_group_size(zgo->pbuddygroup, (gboolean)TRUE));
}
/* }}} */


/* {{{ proto int PhurpleBuddyGroup::getOnlineCount(void)
	gets the count of the buddies in the group with the status online*/
PHP_METHOD(PhurpleBuddyGroup, getOnlineCount)
{
	struct ze_buddygroup_obj *zgo;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zgo = (struct ze_buddygroup_obj *) zend_object_store_get_object(getThis() TSRMLS_CC);

	RETURN_LONG(purple_blist_get_group_online_count(zgo->pbuddygroup));
}
/* }}} */


/* {{{ proto string PhurpleBuddyGroup::getName(void)
	gets the name of the group */
PHP_METHOD(PhurpleBuddyGroup, getName)
{
	struct ze_buddygroup_obj *zgo;
	char *name;

	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	zgo = (struct ze_buddygroup_obj *) zend_object_store_get_object(getThis() TSRMLS_CC);

	name = purple_group_get_name(zgo->pbuddygroup);
	if(name) {
		RETURN_STRING(name, 1);
	}

	RETURN_NULL();
}
/* }}} */

/*
**
**
** End phurple BuddyGroup methods
**
*/
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
