// see note in ../src/evolution2_sync.h
#define HANDLE_LIBICAL_MEMORY 1
#include <glib.h>
#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libedataserver/e-data-server-util.h>


void print_sources(ESourceList *sources)
{
	ESource *source = NULL;

	GSList *g = NULL;
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		printf("Group: %s\n", e_source_group_peek_name(group));

		GSList *s = NULL;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			source = E_SOURCE (s->data);
			printf("  %s: %s\n", e_source_peek_name(source), e_source_get_uri(source));
		}
	}
}

void print_calendars(ECalSourceType source_type) {
	ESourceList *sources = NULL;
	if (e_cal_get_sources(&sources, source_type, NULL)) {
		print_sources(sources);
	}
}

void print_memos()
{
	print_calendars(E_CAL_SOURCE_TYPE_JOURNAL);
}

void print_tasks()
{
	print_calendars(E_CAL_SOURCE_TYPE_TODO);
}

void print_events()
{
	print_calendars(E_CAL_SOURCE_TYPE_EVENT);
}


int main () {
	ESourceList *sources = NULL;
	printf("Addressbooks:\n");
	if (e_book_get_addressbooks(&sources, NULL)) {
		print_sources(sources);
	}
	printf("\n");
	printf("Events:\n");
	print_events();
	printf("\n");

	printf("Tasks:\n");
	print_tasks();
	printf("\n");
	
	printf("Memos:\n");
	print_memos();


	return 0;
}
