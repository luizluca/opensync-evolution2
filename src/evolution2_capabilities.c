/*
 * evo2-sync - A plugin for the opensync framework
 * Copyright (C) 2008 Ian Martin <ianmartin@cantab.net>
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

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>

#include <glib.h>

#include "evolution2_capabilities.h"

#define cast_g_hash_table_insert(hash, a, b) g_hash_table_insert(hash, (gpointer) a, (gpointer) b)

static GHashTable* get_ebook_to_xmlformat_hash()
{
	osync_trace(TRACE_ENTRY, "%s", __func__);
	GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);

	cast_g_hash_table_insert(hash, "address", "Address");
	cast_g_hash_table_insert(hash, "address_label_home", "AddressLabel");
	cast_g_hash_table_insert(hash, "address_label_other", "AddressLabel");
	cast_g_hash_table_insert(hash, "address_label_work", "AddressLabel");
	cast_g_hash_table_insert(hash, "anniversary", "Anniversary");
	cast_g_hash_table_insert(hash, "assistant", "Assistant");
	cast_g_hash_table_insert(hash, "birth_date", "Birthday");
	cast_g_hash_table_insert(hash, "blog_url", "BlogUrl");
	cast_g_hash_table_insert(hash, "caluri", "CalendarUrl");
	cast_g_hash_table_insert(hash, "categories", "Categories");
	/* cast_g_hash_table_insert(hash, "", "Class");*/
	cast_g_hash_table_insert(hash, "email_1", "EMail");
	cast_g_hash_table_insert(hash, "email_2", "EMail");
	cast_g_hash_table_insert(hash, "email_3", "EMail");
	cast_g_hash_table_insert(hash, "email_4", "EMail");
	cast_g_hash_table_insert(hash, "email", "EMail");
	cast_g_hash_table_insert(hash, "file_as", "FileAs");
	cast_g_hash_table_insert(hash, "full_name", "FormattedName");
	cast_g_hash_table_insert(hash, "fburl", "FreeBusyUrl");
	/*  cast_g_hash_table_insert(hash, "", "GroupwiseDirectory");*/
	cast_g_hash_table_insert(hash, "im_aim", "IM-AIM");
	cast_g_hash_table_insert(hash, "im_gadugadu", "IM-GaduGadu");
	cast_g_hash_table_insert(hash, "im_icq", "IM-ICQ");
	cast_g_hash_table_insert(hash, "im_jabber", "IM-Jabber");
	cast_g_hash_table_insert(hash, "im_msn", "IM-MSN");
	/* cast_g_hash_table_insert(hash, "", "IM-Yabber");*/
	cast_g_hash_table_insert(hash, "im_yahoo", "IM-Yahoo");
	/* cast_g_hash_table_insert(hash, "", "IRC");*/
	/* cast_g_hash_table_insert(hash, "", "KDE-Extension");*/
	/* cast_g_hash_table_insert(hash, "", "Key");*/
	/* cast_g_hash_table_insert(hash, "", "Location");*/
	cast_g_hash_table_insert(hash, "logo", "Logo");
	cast_g_hash_table_insert(hash, "manager", "Manager");
	cast_g_hash_table_insert(hash, "name", "Name");
	cast_g_hash_table_insert(hash, "nickname", "Nickname");
	cast_g_hash_table_insert(hash, "note", "Note");
	cast_g_hash_table_insert(hash, "org", "Organization");
	cast_g_hash_table_insert(hash, "photo", "Photo");
	/*cast_g_hash_table_insert(hash, "", "PhotoUrl");*/

	/*To match the vcard translators view that evolution role is really profession */
	cast_g_hash_table_insert(hash, "role", "Profession");
	cast_g_hash_table_insert(hash, "Rev", "Revision");
	/* cast_g_hash_table_insert(hash, "", "Role"); Evolution's role is profession */
	/* cast_g_hash_table_insert(hash, "", "SMS");*/
	/* cast_g_hash_table_insert(hash, "", "Sound");*/
	cast_g_hash_table_insert(hash, "spouse", "Spouse");
	cast_g_hash_table_insert(hash, "phone", "Telephone");
	cast_g_hash_table_insert(hash, "title", "Title");
	cast_g_hash_table_insert(hash, "id", "Uid");
	cast_g_hash_table_insert(hash, "homepage_url", "Url");
	/* cast_g_hash_table_insert(hash, "", "UserDefined");*/
	/* cast_g_hash_table_insert(hash, "", "Version");*/
	cast_g_hash_table_insert(hash, "video_url", "VideoUrl");
	cast_g_hash_table_insert(hash, "wants_html", "WantsHtml");

	osync_trace(TRACE_EXIT, "%s", __func__);
	return hash;
}

osync_bool evo2_translate_capabilities(OSyncCapabilities *caps, GHashTable *hash, GList *fields, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %s, %p)", __func__, caps, hash, fields, objtype, error);
	osync_assert(caps);
	osync_assert(hash);
	osync_assert(fields);
	osync_assert(objtype);

	for(; fields; fields = g_list_next(fields)) {

		const char *value = g_hash_table_lookup(hash, fields->data);
		if (value == NULL) {
			osync_trace(TRACE_INTERNAL, "%s: ebook capability '%s' could not be translated", __func__, fields->data);
		} else {
			osync_trace(TRACE_INTERNAL, "%s: ebook capability '%s' translated to '%s'", __func__, fields->data, value);
			/* TODO Check for uniqueness of the added capability as several fields
			   translate to the same capability */
			if (!osync_capability_new(caps, objtype, value, error)) {
				goto error;
			}
		}
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

 error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool evo2_capbilities_translate_ebook(OSyncCapabilities *caps, GList *fields, OSyncError **error) {
	GHashTable *hash;
	hash = get_ebook_to_xmlformat_hash();
	return evo2_translate_capabilities(caps, hash, fields, "contact", error);
}
