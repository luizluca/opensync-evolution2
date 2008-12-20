#ifndef ECAL_H
#define ECAL_H

#include <opensync/opensync.h>
#include <opensync/opensync-merger.h>

#include "evolution2_sync.h"

osync_bool evo2_ecal_initialize(OSyncEvoEnv *env, OSyncPluginInfo *info, OSyncError **error);
osync_bool evo2_ecal_discover(OSyncEvoEnv *env, OSyncCapabilities *caps, OSyncError **error);
#endif /*  ECAL_H */
