#ifndef EVO2_SYNC_H
#define EVO2_SYNC_H

//
// Some versions of libical use a ring buffer for the following
// functions, for which the library manages memory.  Somewhere along
// the line, this was fixed so that the application was responsible
// for freeing the strings returned by these functions.  A warning
// was added to the header that would display if HANDLE_LIBICAL_MEMORY
// was not defined.
//
// Even newer versions of evolution-data-server, which this plugin
// depends on, get rid of some of these functions, while newer
// versions of libical add _r variants that implement the
// application-free functinality.
//
// Since this plugin does not use these functions, we disable the
// warning by defining HANDLE_LIBICAL_MEMORY, then include the
// headers, and then purposely break the troublesome functions
// so that if a programmer tries to use them later on, he'll know
// to handle them with care.
//
#define HANDLE_LIBICAL_MEMORY 1

#include <opensync/opensync.h>

#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libedataserver/e-data-server-util.h>

#define icalreqstattype_as_string() See_evolution2_sync_h_for_note
#define icalproperty_as_ical_string() See_evolution2_sync_h_for_note
#define icalproperty_get_parameter_as_string() See_evolution2_sync_h_for_note
#define icalproperty_get_value_as_string() See_evolution2_sync_h_for_note
#define icallangbind_property_eval_string() See_evolution2_sync_h_for_note
#define icalperiodtype_as_ical_string() See_evolution2_sync_h_for_note
#define icaltime_as_ical_string() See_evolution2_sync_h_for_note
#define icalvalue_as_ical_string() See_evolution2_sync_h_for_note
#define icalcomponent_as_ical_string() See_evolution2_sync_h_for_note
#define e_cal_component_get_recurid_as_string() See_evolution2_sync_h_for_note


#define STR_URI_KEY		"uri_"


typedef struct OSyncEvoCalendar {
	char *uri_key;
	const char *uri;
	const char *objtype;
	const char *change_id;
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

ESource *evo2_find_source(ESourceList *list, const char *uri);

#endif
