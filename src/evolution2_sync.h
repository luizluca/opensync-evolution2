#ifndef EVO2_SYNC_H
#define EVO2_SYNC_H

//#include "evo2_sync.h"

#include <opensync/opensync.h>

#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libedataserver/e-data-server-util.h>

typedef struct evo2_location {
	char *name;
	char *uri;
} evo2_location;

typedef struct OSyncEvoCalendar {
	const char *uri;
	const char *objtype;
	ECalSourceType source_type;
	icalcomponent_kind ical_component;
	ECal *calendar;
	OSyncObjTypeSink *sink;
	OSyncObjFormat *format;
} OSyncEvoCalendar;

typedef struct OSyncEvoEnv {
	char *change_id;
	
	const char *addressbook_path;
	EBook *addressbook;
	OSyncObjTypeSink *contact_sink;
	OSyncObjFormat *contact_format;
	
	GList *calendars;

	OSyncPluginInfo *pluginInfo;	
} OSyncEvoEnv;

ESource *evo2_find_source(ESourceList *list, char *uri);

#endif
