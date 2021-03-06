/*
 * Fimex, testFileReaderFactory.cc
 *
 * (C) Copyright 2010, met.no
 *
 * Project Info:  https://wiki.met.no/fimex/start
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 *  Created on: May 7, 2010
 *      Author: Heiko Klein
 */

#include "testinghelpers.h"
#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include "fimex/CDMFileReaderFactory.h"
#include "fimex/CDMconstants.h"
#ifdef HAVE_NETCDF_H
#define MIFI_IO_READER_SUPPRESS_DEPRECATED
#include "fimex/NetCDF_CDMReader.h"
#endif

using namespace std;
using namespace MetNoFimex;

BOOST_AUTO_TEST_CASE( test_CDMconstants )
{
    string nc(mifi_get_filetype_name(MIFI_FILETYPE_NETCDF));
    BOOST_CHECK(nc == "netcdf");
    int nc_id = mifi_get_filetype(nc.c_str());
    BOOST_CHECK(nc_id == MIFI_FILETYPE_NETCDF);
#ifdef HAVE_NETCDF_H
    BOOST_CHECK(fimexHas(MIFI_FILETYPE_NETCDF));
#else
    BOOST_CHECK(fimexHas(MIFI_FILETYPE_NETCDF) == 0);
#endif
}

BOOST_AUTO_TEST_CASE( test_fileDetection )
{
    const string fileName = pathTest("coordTest.nc");
    BOOST_CHECK(CDMFileReaderFactory::detectFileType(fileName) == MIFI_FILETYPE_NETCDF);
    if (hasTestExtra()) {
        const std::string feltFile = pathTestExtra("flth00.dat");
        BOOST_CHECK(CDMFileReaderFactory::detectFileType(feltFile) == MIFI_FILETYPE_FELT);
    }
    if (fimexHas(MIFI_FILETYPE_NETCDF)) {
        CDMReader_p reader = CDMFileReaderFactory::create("netcdf", fileName);
        BOOST_CHECK(reader.get() != 0);
#ifdef HAVE_NETCDF_H
        BOOST_CHECK(dynamic_cast<NetCDF_CDMReader*>(reader.get()) != 0);
#endif
    }
}

#endif // HAVE_BOOST_UNIT_TEST_FRAMEWORK
