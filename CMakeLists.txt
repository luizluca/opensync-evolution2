CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )
PROJECT( libopensync-plugin-evolution2 C )

SET( VERSION "0.40" )

SET( OPENSYNC_MIN_VERSION "0.40" )

FIND_PACKAGE( EPackage REQUIRED )
FIND_PACKAGE( OpenSync REQUIRED )
FIND_PACKAGE( GLIB2 REQUIRED )


INCLUDE( OpenSyncInternal )
INCLUDE( Testing )

INCLUDE( CheckIncludeFile )
CHECK_INCLUDE_FILE( "libedataserver/eds-version.h" HAVE_EDS_VERSION_H )

ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( tools )
ADD_SUBDIRECTORY( tests )


#############################################

CONFIGURE_FILE( misc/evolution2.spec.in misc/evolution2.spec )

OPENSYNC_PACKAGE( ${PROJECT_NAME} ${VERSION} )

