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

#ifndef EVO2_CAPABILITIES_H
#define EVO2_CAPABILITIES_H
#include <opensync/opensync.h>
#include <opensync/opensync-plugin.h>

#include "evolution2_sync.h"
/*! @brief Function dynamically figures out the capabilities of evolution 
 *
 * Returns True on success
 * @param info Pointer to the plugin info to which the capabilities will be added
 * @param error Error information if return value is False
 */

osync_bool evo2_capbilities_translate_ebook(OSyncCapabilities *caps, GList *fields, OSyncError **error);
#endif /* EVO2_CAPABILITIES_H */

