// see note in ../src/evolution2_sync.h
#define HANDLE_LIBICAL_MEMORY 1
#include <string.h>
#include <glib.h>
#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libedataserver/e-data-server-util.h>

void open_calendar(const char *uri, ECalSourceType source_type)
{
	ECal *cal = NULL;
	GError *gerror = NULL;
	cal = e_cal_new_from_uri(uri, source_type);

	if (!cal) {
		printf("Failed to allocate calendar\n");
		return;
	}
	if (!e_cal_open(cal, TRUE, &gerror)) {
		printf("Failed to open calendar:\n%s\n", gerror->message);
		g_clear_error(&gerror);
		return;
	}
	printf("Successfully opened %s\n", uri);
	g_object_unref(cal);
}

void open_book(const char *uri)
{
	EBook *book = NULL;
	GError *gerror = NULL;
	book = e_book_new_from_uri(uri, &gerror);

	if (book) {
		if (e_book_open(book, TRUE, &gerror)) {
			printf("Successfully opened %s\n", uri);
		}
		g_object_unref(book);
	}
	if (gerror) {
		printf("Failed to open addressbook:\n%s\n", gerror->message);
		g_clear_error(&gerror);
	}
}

static void usage(char *progname, int exitcode)
{
	fprintf (stderr, "Usage: %s <objtype> <uri>\n\n", progname);
	fprintf (stderr, "objtype may be one of:\n");
	fprintf (stderr, "--contact\n");
	fprintf (stderr, "--event\n");
	fprintf (stderr, "--note\n");
	fprintf (stderr, "--todo\n");
	fprintf (stderr, "\nWARNING: This program allows you to attempt to open ECals with the wrong source type.  Doing so may break everything\n");
	exit(exitcode);
}

int main(int argc, char *argv[])
{

	g_type_init();
	if (argc < 2)
		usage(argv[0], 1);
	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
		usage(argv[0], 0);
		    
	if (argc <3)
		usage(argv[0], 1);
	if (!strcmp(argv[1], "--contact")) {
		open_book(argv[2]);
	} else if (!strcmp(argv[1], "--event")) {
		open_calendar(argv[2], E_CAL_SOURCE_TYPE_EVENT);
	} else if (!strcmp(argv[1], "--todo")) {
		open_calendar(argv[2], E_CAL_SOURCE_TYPE_TODO);
	} else if (!strcmp(argv[1], "--note")) {
		open_calendar(argv[2], E_CAL_SOURCE_TYPE_JOURNAL);
	} else {
		usage(argv[0], 1);
	}
	return 0;
}
