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

typedef struct OSyncEvoEnv {
	char *change_id;
	
	const char *addressbook_path;
	EBook *addressbook;
	OSyncObjTypeSink *contact_sink;
	OSyncObjFormat *contact_format;
	
	const char *calendar_path;
	ECal *calendar;
	OSyncObjTypeSink *calendar_sink;
	OSyncObjFormat *calendar_format;
	
	const char *memos_path;
	ECal *memos;
	OSyncObjTypeSink *memos_sink;
	OSyncObjFormat *memos_format;
	
	const char *tasks_path;
	ECal *tasks;
	OSyncObjTypeSink *tasks_sink;
	OSyncObjFormat *tasks_format;

	OSyncPluginInfo *pluginInfo;	
} OSyncEvoEnv;

ESource *evo2_find_source(ESourceList *list, char *uri);

#endif
