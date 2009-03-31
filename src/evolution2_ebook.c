/*
 * evolution2_sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#include <string.h>

#include <opensync/opensync.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-plugin.h>

#include "evolution2_capabilities.h"

#include "evolution2_ebook.h"

EBook *evo2_ebook_open_book(char *path, OSyncError **error) 
{
	EBook *addressbook = NULL;
	GError *gerror = NULL;
	ESourceList *sources = NULL;
	ESource *source = NULL;
	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, path, error);

	if (!path) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "no addressbook path set");
	  	goto error;
	}

	if (strcmp(path, "default")) {
		if (!e_book_get_addressbooks(&sources, &gerror)) {
	  		osync_error_set(error, OSYNC_ERROR_GENERIC, "Error getting addressbooks: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
		
		if (!(source = evo2_find_source(sources, path))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Error finding source \"%s\"", path);
	  		goto error;
		}
		
		if (!(addressbook = e_book_new(source, &gerror))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
	} else {
		osync_trace(TRACE_INTERNAL, "Opening default addressbook\n");
		if (!(addressbook = e_book_new_default_addressbook(&gerror))) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
	  		goto error;
		}
	}

	if (!e_book_open(addressbook, TRUE, &gerror)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to alloc new addressbook: %s", gerror ? gerror->message : "None");
	  	goto error_free_book;
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return addressbook;


 error_free_book:
	g_object_unref(addressbook);
 error:
	if (gerror)
		g_clear_error(&gerror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;

}

osync_bool evo2_ebook_discover(OSyncEvoEnv *env, OSyncCapabilities *caps, OSyncError **error) 
{
	EBook *book = NULL;
	GList *fields = NULL;
	GError *gerror = NULL;
	gboolean success;
	gboolean writable;

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, caps, error);
	osync_assert(env);
	osync_assert(caps);

	if (env->contact_sink) {
		if (!(book = evo2_ebook_open_book(g_strdup(env->addressbook_path), error))) {
			goto error;
		}
		writable = e_book_is_writable(book);
		osync_objtype_sink_set_write(env->contact_sink, writable);
		osync_trace(TRACE_INTERNAL, "Set sink write status to %s", writable ? "TRUE" : "FALSE");

		success = e_book_get_supported_fields (book, &fields, &gerror);
		g_object_unref(book);
		if (!success) {
			osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to get supported fields: %s", gerror ? gerror->message : "None");
			goto error;
		}
		
		success = evo2_capbilities_translate_ebook(caps, fields, error);
		while (fields) {
			g_free(fields->data);
			fields = g_list_remove(fields, fields->data);
		}
		if (!success) {
			goto error;
		}
	}
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
 error:
	if (gerror)
		g_clear_error(&gerror);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;

}

static void evo2_ebook_connect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	OSyncError *error = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, info, ctx, userdata);
	OSyncEvoEnv *env = (OSyncEvoEnv *)userdata;
	osync_bool state_match;

	if (!(env->addressbook = evo2_ebook_open_book(osync_strdup(env->addressbook_path), &error))) {
		goto error;
	}
	
	OSyncSinkStateDB *state_db = osync_objtype_sink_get_state_db(sink);
	if (!state_db) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "State database missing for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error_free_book;
	}
	if (!osync_sink_state_equal(state_db, "path", env->addressbook_path, &state_match, &error)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Anchor comparison failed for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error_free_book;
	}
	if (!state_match) {
		osync_trace(TRACE_INTERNAL, "EBook slow sync, due to anchor mismatch");
		osync_context_report_slowsync(ctx);
	}

	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

 error_free_book:
	g_object_unref(env->addressbook);
	env->addressbook = NULL;
 error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void evo2_ebook_disconnect(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, userdata, info, ctx);
	OSyncEvoEnv *env = (OSyncEvoEnv *)userdata;
	
	if (env->addressbook) {
		g_object_unref(env->addressbook);
		env->addressbook = NULL;
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

static void evo2_ebook_sync_done(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, sink, info, ctx, userdata);
	OSyncEvoEnv *env = (OSyncEvoEnv *)userdata;
	OSyncError *error = NULL;
	GError *gerror=NULL;

	OSyncSinkStateDB *state_db = osync_objtype_sink_get_state_db(sink);
	if (!state_db) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "State database missing for objtype \"%s\"", osync_objtype_sink_get_name(sink));
		goto error;
	}
	if (!osync_sink_state_set(state_db, "path", env->addressbook_path, &error))
		goto error;
	
	GList *changes = NULL;
	if (!e_book_get_changes(env->addressbook, env->change_id, &changes, &gerror)) {
		osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to update EBook time of last sync: %s", gerror ? gerror->message : "None");
		g_clear_error(&gerror);
		goto error;
	}

	e_book_free_change_list(changes);
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

 error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
	
}

void evo2_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype)
{
	OSyncError *error = NULL;
	
	OSyncChange *change = osync_change_new(&error);
	if (!change) {
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		return;
	}
	
	osync_change_set_uid(change, uid);
	osync_change_set_changetype(change, changetype);
	
	OSyncData *odata = osync_data_new(data, size, format, &error);
	if (!odata) {
		osync_change_unref(change);
		osync_context_report_osyncwarning(ctx, error);
		osync_error_unref(&error);
		return;
	}
	
	osync_change_set_data(change, odata);
	osync_data_unref(odata);

	osync_context_report_change(ctx, change);
	
	osync_change_unref(change);
}

static void evo2_ebook_get_changes(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, osync_bool slow_sync, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, sink, info, ctx, slow_sync ? "TRUE" : "FALSE", userdata);
	OSyncEvoEnv *env = (OSyncEvoEnv *)userdata;
	OSyncError *error = NULL;
	
	GList *changes = NULL;
	EBookChange *ebc = NULL;
	EVCard vcard;
	GList *l = NULL;
	char *data = NULL;
	char *uid = NULL;
	int datasize = 0;
	GError *gerror = NULL;
	
	if (slow_sync == FALSE) {
		osync_trace(TRACE_INTERNAL, "No slow_sync for contact");
		if (!e_book_get_changes(env->addressbook, env->change_id, &changes, &gerror)) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to alloc new default addressbook: %s", gerror ? gerror->message : "None");
			goto error;
		}
		osync_trace(TRACE_INTERNAL, "Found %i changes for change-ID %s", g_list_length(changes), env->change_id);
		
		for (l = changes; l; l = l->next) {
			ebc = (EBookChange *)l->data;
			uid = g_strdup(e_contact_get_const(ebc->contact, E_CONTACT_UID));
			e_contact_set(ebc->contact, E_CONTACT_UID, NULL);
			switch (ebc->change_type) {
				case E_BOOK_CHANGE_CARD_ADDED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
					break;
				case E_BOOK_CHANGE_CARD_MODIFIED:
					vcard = ebc->contact->parent;
					data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
					datasize = strlen(data) + 1;
					evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_MODIFIED);
					break;
				case E_BOOK_CHANGE_CARD_DELETED:
					evo2_report_change(ctx, env->contact_format, NULL, 0, uid, OSYNC_CHANGE_TYPE_DELETED);
					break;
			}
			g_free(uid);
		}
	} else {
		osync_trace(TRACE_INTERNAL, "slow_sync for contact");
		EBookQuery *query = e_book_query_any_field_contains("");
		if (!e_book_get_contacts(env->addressbook, query, &changes, &gerror)) {
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Failed to get changes from addressbook: %s", gerror ? gerror->message : "None");
			goto error;
		}
		for (l = changes; l; l = l->next) {
			EContact *contact = E_CONTACT(l->data);
			vcard = contact->parent;
			data = e_vcard_to_string(&vcard, EVC_FORMAT_VCARD_30);
			uid = g_strdup(e_contact_get_const(contact, E_CONTACT_UID));
			datasize = strlen(data) + 1;
			evo2_report_change(ctx, env->contact_format, data, datasize, uid, OSYNC_CHANGE_TYPE_ADDED);
			g_free(uid);
		}
		e_book_query_unref(query);
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (gerror)
		g_clear_error(&gerror);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void evo2_ebook_modify(OSyncObjTypeSink *sink, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change, void *userdata)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p, %p)", __func__, sink, info, ctx, change, userdata);
	OSyncEvoEnv *env = (OSyncEvoEnv *)userdata;
	
	const char *uid = osync_change_get_uid(change);
	EContact *contact = NULL;
	GError *gerror = NULL;
	OSyncError *error = NULL;
	OSyncData *odata = NULL;
	char *plain = NULL;
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			if (!e_book_remove_contact(env->addressbook, uid, &gerror)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete contact: %s", gerror ? gerror->message : "None");
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			contact = e_contact_new_from_vcard(plain);
			e_contact_set(contact, E_CONTACT_UID, NULL);
			if (e_book_add_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const(contact, E_CONTACT_UID);
				osync_change_set_uid(change, uid);
			} else {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to add contact: %s", gerror ? gerror->message : "None");
				goto error;
			}
			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			odata = osync_change_get_data(change);
			osync_data_get_data(odata, &plain, NULL);
			
			contact = e_contact_new_from_vcard(plain);
			e_contact_set(contact, E_CONTACT_UID, g_strdup(uid));
			
			osync_trace(TRACE_INTERNAL, "ABout to modify vcard:\n%s", e_vcard_to_string(&(contact->parent), EVC_FORMAT_VCARD_30));
			
			if (e_book_commit_contact(env->addressbook, contact, &gerror)) {
				uid = e_contact_get_const (contact, E_CONTACT_UID);
				if (uid)
					osync_change_set_uid(change, uid);
			} else {
				/* try to add */
				osync_trace(TRACE_INTERNAL, "unable to mod contact: %s", gerror ? gerror->message : "None");
				
				g_clear_error(&gerror);
				if (e_book_add_contact(env->addressbook, contact, &gerror)) {
					uid = e_contact_get_const(contact, E_CONTACT_UID);
					osync_change_set_uid(change, uid);
				} else {
					osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to modify contact: %s", gerror ? gerror->message : "None");
					goto error;
				}
			}
			break;
		default:
			printf("Error\n");
	}
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	if (gerror)
		g_clear_error(&gerror);
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}


osync_bool evo2_ebook_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, info, error);
	OSyncObjTypeSink *sink = osync_plugin_info_find_objtype(info, "contact");
	if (!sink) {
		osync_trace(TRACE_INTERNAL, "No sink for objtype contact, ebook not initialized");
		return TRUE;
	}
	osync_bool sinkEnabled = osync_objtype_sink_is_enabled(sink);
        osync_trace(TRACE_INTERNAL, "%s: enabled => %d", __func__, sinkEnabled);
	if (!sinkEnabled) {
		osync_trace(TRACE_INTERNAL, "Sink for objtype contact not enabled, ebook not initialized");
		return TRUE;
	}
	
	osync_objtype_sink_set_connect_func(sink, evo2_ebook_connect);
	osync_objtype_sink_set_disconnect_func(sink, evo2_ebook_disconnect);
	osync_objtype_sink_set_get_changes_func(sink, evo2_ebook_get_changes);
	osync_objtype_sink_set_commit_func(sink, evo2_ebook_modify);
	osync_objtype_sink_set_sync_done_func(sink, evo2_ebook_sync_done);

	osync_objtype_sink_enable_state_db(sink, TRUE);

	OSyncPluginConfig *config = osync_plugin_info_get_config(info);
	OSyncPluginResource *resource = osync_plugin_config_find_active_resource(config, "contact");
	env->addressbook_path = osync_plugin_resource_get_url(resource);
	if(!env->addressbook_path) {
		osync_error_set(error,OSYNC_ERROR_GENERIC, "Addressbook url not set");
		goto error;
	}
	OSyncList *objformatsinks = osync_plugin_resource_get_objformat_sinks(resource);
	osync_bool hasObjFormat = FALSE;
	OSyncList *r;
	for(r = objformatsinks;r;r = r->next) {
		OSyncObjFormatSink *objformatsink = r->data;
		if(!strcmp("vcard30", osync_objformat_sink_get_objformat(objformatsink))) { hasObjFormat = TRUE; break;}
	}
        if (!hasObjFormat) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Format vcard30 not set.");
		goto error;
	}

	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	env->contact_format = osync_format_env_find_objformat(formatenv, "vcard30");
	assert(env->contact_format);

	env->contact_sink = osync_objtype_sink_ref(sink);

	osync_objtype_sink_set_userdata(sink, env);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

 error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

