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
#include <opensync/opensync-capabilities.h>

#include <glib.h>

#include "evolution2_capabilities.h"

osync_bool evo2_translate_capabilities(OSyncCapabilities *caps, GList *fields, const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %s, %p)", __func__, caps, fields, objtype, error);
	osync_assert(caps);
	osync_assert(fields);
	osync_assert(objtype);

	OSyncCapabilitiesObjType *capsobjtype = osync_capabilities_objtype_new(caps, objtype, error);

	for(; fields; fields = g_list_next(fields)) {

		OSyncCapability *cap = osync_capability_new(capsobjtype, error);
		if (!cap)
			goto error;

		osync_capability_set_name(cap, (const char *) fields->data);
	}

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

 error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool evo2_capbilities_translate_ebook(OSyncCapabilities *caps, GList *fields, OSyncError **error) {
	return evo2_translate_capabilities(caps, fields, "contact", error);
}
