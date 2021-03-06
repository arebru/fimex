#ifndef CDMCONSTANTS_H_
#define CDMCONSTANTS_H_

#include "fimex/deprecated.h"

/**
 * @headerfile fimex/CDMconstants.h
 */
/**
 * @brief constants used through-out fimex
 *
 * CDMConstants stores several constants used in fimex, accessible from
 * C and C++. Constants are either available as macro, or as function.
 */

#define MIFI_STRINGIFY1(x) #x
#define MIFI_STRINGIFY(x) MIFI_STRINGIFY1(x)

/*
 * The following numbers are used by configure.ac
 * - make sure the line-number is correct
 */
/*
 * fimex version status, e.g >=0xF0 = final, 0xAX = alphaX,
 * 0xBX = betaX, 0xCX= releaseCandidateX
 * touch configure.ac after changing these numbers
 */
#define MIFI_VERSION_MAJOR  0
#define MIFI_VERSION_MINOR 67
#define MIFI_VERSION_PATCH  2
#define MIFI_VERSION_STATUS 0xF0

#define MIFI_VERSION_PATCH_STRING "." MIFI_STRINGIFY(MIFI_VERSION_PATCH)

#if (MIFI_VERSION_STATUS == 0xA1)
#define MIFI_VERSION_STATUS_STRING "~alpha1"
#elif (MIFI_VERSION_STATUS == 0xA2)
#define MIFI_VERSION_STATUS_STRING "~alpha2"
#elif (MIFI_VERSION_STATUS == 0xA3)
#define MIFI_VERSION_STATUS_STRING "~alpha3"
#elif (MIFI_VERSION_STATUS == 0xA4)
#define MIFI_VERSION_STATUS_STRING "~alpha4"
#elif (MIFI_VERSION_STATUS == 0xA5)
#define MIFI_VERSION_STATUS_STRING "~alpha5"
#elif (MIFI_VERSION_STATUS == 0xA6)
#define MIFI_VERSION_STATUS_STRING "~alpha6"
#elif (MIFI_VERSION_STATUS == 0xA7)
#define MIFI_VERSION_STATUS_STRING "~alpha7"
#elif (MIFI_VERSION_STATUS == 0xA8)
#define MIFI_VERSION_STATUS_STRING "~alpha8"
#elif (MIFI_VERSION_STATUS == 0xA9)
#define MIFI_VERSION_STATUS_STRING "~alpha9"

#elif (MIFI_VERSION_STATUS == 0xB1)
#define MIFI_VERSION_STATUS_STRING "~beta1"
#elif (MIFI_VERSION_STATUS == 0xB2)
#define MIFI_VERSION_STATUS_STRING "~beta2"
#elif (MIFI_VERSION_STATUS == 0xB3)
#define MIFI_VERSION_STATUS_STRING "~beta3"
#elif (MIFI_VERSION_STATUS == 0xB4)
#define MIFI_VERSION_STATUS_STRING "~beta4"
#elif (MIFI_VERSION_STATUS == 0xB5)
#define MIFI_VERSION_STATUS_STRING "~beta5"
#elif (MIFI_VERSION_STATUS == 0xB6)
#define MIFI_VERSION_STATUS_STRING "~beta6"
#elif (MIFI_VERSION_STATUS == 0xB7)
#define MIFI_VERSION_STATUS_STRING "~beta7"
#elif (MIFI_VERSION_STATUS == 0xB8)
#define MIFI_VERSION_STATUS_STRING "~beta8"
#elif (MIFI_VERSION_STATUS == 0xB9)
#define MIFI_VERSION_STATUS_STRING "~beta9"

#elif (MIFI_VERSION_STATUS == 0xC1)
#define MIFI_VERSION_STATUS_STRING "~rc1"
#elif (MIFI_VERSION_STATUS == 0xC2)
#define MIFI_VERSION_STATUS_STRING "~rc2"
#elif (MIFI_VERSION_STATUS == 0xC3)
#define MIFI_VERSION_STATUS_STRING "~rc3"
#elif (MIFI_VERSION_STATUS == 0xC4)
#define MIFI_VERSION_STATUS_STRING "~rc4"
#elif (MIFI_VERSION_STATUS == 0xC5)
#define MIFI_VERSION_STATUS_STRING "~rc5"
#elif (MIFI_VERSION_STATUS == 0xC6)
#define MIFI_VERSION_STATUS_STRING "~rc6"
#elif (MIFI_VERSION_STATUS == 0xC7)
#define MIFI_VERSION_STATUS_STRING "~rc7"
#elif (MIFI_VERSION_STATUS == 0xC8)
#define MIFI_VERSION_STATUS_STRING "~rc8"
#elif (MIFI_VERSION_STATUS == 0xC9)
#define MIFI_VERSION_STATUS_STRING "~rc9"

#elif (MIFI_VERSION_STATUS == 0xF0)
#define MIFI_VERSION_STATUS_STRING ""

#else
#error "unknown MIFI_VERSION_STATUS"
#endif

#define MIFI_VERSION_STRING \
    MIFI_STRINGIFY(MIFI_VERSION_MAJOR )\
    "." MIFI_STRINGIFY(MIFI_VERSION_MINOR) \
    MIFI_VERSION_PATCH_STRING \
    MIFI_VERSION_STATUS_STRING

#define MIFI_VERSION_INT(major,minor,patch) \
    (1000000*major + 1000*minor + patch)
#define MIFI_VERSION_CURRENT_INT \
    MIFI_VERSION_INT(MIFI_VERSION_MAJOR, MIFI_VERSION_MINOR, MIFI_VERSION_PATCH)

/**
 * the default radius of a spherical earth in meter
 */
#define MIFI_EARTH_RADIUS_M 6371000

/**
 * the default earth projection (WGS84 latlong)
 */
#define MIFI_WGS84_LATLON_PROJ4 "+proj=latlong +datum=WGS84 +towgs84=0,0,0 +no_defs"

/**
 * The #mifi_filetype define the available input and output file-formats.
 * Maximum 1023.
 */
/* when changing, remember to update CDMconstants.cc#getFileTypeNames,
 * make sure that the maximum number is <= size of filetypes */
enum mifi_filetype {
    /**
     * only filetype < 0
     */
    MIFI_FILETYPE_UNKNOWN=-1,
    MIFI_FILETYPE_FELT,
    MIFI_FILETYPE_NETCDF,
    MIFI_FILETYPE_NCML,
    MIFI_FILETYPE_GRIB,
    MIFI_FILETYPE_WDB,
    MIFI_FILETYPE_METGM,
    MIFI_FILETYPE_PRORAD,
    MIFI_FILETYPE_GRBML,
    /**
     * MIFI_FILETYPE_RW is a flag to specify that files should be opened in read-write mode.
     * Example: MIFI_FILETYPE_NETCDF|MIFI_FILETYPE_RW
     */
    MIFI_FILETYPE_RW=1024
};

/**
 * default fill values taken from netcdf.h
 */
#define MIFI_FILL_CHAR    ((signed char)-127)
#define MIFI_FILL_SHORT   ((short)-32767)
#define MIFI_FILL_INT     (-2147483647L)
#define MIFI_FILL_FLOAT   (9.9692099683868690e+36f) /* near 15 * 2^119 */
#define MIFI_FILL_DOUBLE  (9.9692099683868690e+36)
#define MIFI_FILL_UCHAR   (255)
#define MIFI_FILL_USHORT  (65535)
#define MIFI_FILL_UINT    (4294967295U)
#define MIFI_FILL_INT64   ((long long)-9223372036854775806LL)
#define MIFI_FILL_UINT64  ((unsigned long long)18446744073709551614ULL)


#ifdef __cplusplus
extern "C" {
#endif

/**
 * version of fimex
 */
extern const char* fimexVersion();

/**
 * major version of fimex
 */
extern unsigned int mifi_version_major();

/**
 * minor version of fimex
 */
extern unsigned int mifi_version_minor();

/**
 * patch version of fimex
 */
extern unsigned int mifi_version_patch();

/**
 * fimex version status, e.g >=0xF0 = final, 0xAX = alphaX,
 * 0xBX = betaX, 0xCX= releaseCandidateX
 */
extern unsigned int mifi_version_status();


/**
 * @brief get the filetype of a filetype name
 * @return one of #mifi_filetype
 */
extern int mifi_get_filetype(const char* filetypeName);

/**
 * @brief get the filetype-name of a filetype
 * @param filetype one of #mifi_filetype
 */
extern const char* mifi_get_filetype_name(int filetype);

/**
 * get the maximum number of filetypes, that is , the largest number
 * of valid filetype you can get.
 */
extern int mifi_get_max_filetype_number();

/**
 * check if fimex is configured with the filetype
 * @param fileType one of the #mifi_filetype define constants
 */
extern int fimexHas(int fileType);
/**
 * check if fimex is configured with netcdf-support
 * @deprecated use fimexHas(fileType)
 */
MIFI_DEPRECATED(int fimexHasNetcdf());
/**
 * check if fimex is configured with grib_api-support
 * @deprecated use fimexHas(fileType)
 */
MIFI_DEPRECATED(int fimexHasGribApi());
/**
 * check if fimex is configured with felt-support
 * @deprecated use fimexHas(fileType)
 */
MIFI_DEPRECATED(int fimexHasFelt());

#ifdef __cplusplus
}
#endif


#endif /*CDMCONSTANTS_H_*/
