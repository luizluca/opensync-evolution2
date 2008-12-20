#ifndef EBOOK_H
#define EBOOK_H

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-plugin.h>

#include "evolution2_sync.h"

osync_bool evo2_ebook_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error);
osync_bool evo2_ebook_discover(OSyncEvoEnv *env, OSyncCapabilities *caps, OSyncError **error);

#endif /*  EBOOK_H */
