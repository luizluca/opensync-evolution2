#ifndef ECAL_H
#define ECAL_H

#include <opensync/opensync.h>
#include <opensync/opensync-capabilities.h>

#include "evolution2_sync.h"

osync_bool evo2_ecal_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, const char *objtype, const char *required_format, OSyncError **error);
osync_bool evo2_ecal_discover(OSyncEvoCalendar *evo_cal, OSyncCapabilities *caps, OSyncError **error);
#endif /*  ECAL_H */
