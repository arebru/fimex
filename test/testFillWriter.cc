/*
 * Fimex, testFillWriter.cc
 *
 * (C) Copyright 2013, met.no
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
 *  Created on: Mar 25, 2013
 *      Author: heikok
 */

#include "testinghelpers.h"
#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include "fimex/FillWriter.h"
#include "fimex/CDMFileReaderFactory.h"
#include "fimex/CDM.h"
#include "fimex/Data.h"

using namespace std;
using namespace MetNoFimex;

BOOST_AUTO_TEST_CASE( test_fillWriter )
{
    const string fileName = pathTest("coordRefTimeTest.nc");
    const string inFillName = pathTest("fillIn2.nc");
    const string outFile = "fillOut.nc";
    copyFile(fileName, outFile);

    {
        CDMReader_p in = CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, inFillName);
        CDMReaderWriter_p out = CDMFileReaderFactory::createReaderWriter(MIFI_FILETYPE_NETCDF, outFile);

        // sanity check before test
        BOOST_CHECK(out->getCDM().getDimension("refTime").getLength() == 2);
        BOOST_CHECK(out->getCDM().getDimension("sigma").getLength() == 4);

        BOOST_CHECK(in->getCDM().getDimension("refTime").getLength() == 2);
        BOOST_CHECK(in->getCDM().getDimension("sigma").getLength() == 2);

        FillWriter(in, out);
    // close the writer before re-reading it
    }
    {
        CDMReader_p out = CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, outFile);

        BOOST_CHECK(out->getCDM().getDimension("refTime").getLength() == 3);
        BOOST_CHECK(out->getCDM().getDimension("sigma").getLength() == 4);
        BOOST_CHECK(out->getData("sigma")->asInt()[0] ==  300);
        BOOST_CHECK(out->getData("sigma")->asInt()[3] == 1000);
        BOOST_CHECK(out->getData("refTime")->asInt()[0] ==  12);
        BOOST_CHECK(out->getData("refTime")->asInt()[2] == 36);

        BOOST_CHECK(out->getDataSlice("cloud_area_fraction_in_atmosphere_layer",0)->asInt()[11*11*2] == -32767); // first number, 3rd level
        BOOST_CHECK(out->getDataSlice("cloud_area_fraction_in_atmosphere_layer",2)->asInt()[11*11*4] == -32767); // last level, last number, first offsetTime
        BOOST_CHECK(out->getDataSlice("cloud_area_fraction_in_atmosphere_layer",2)->asInt()[11*11*4*2 - 1] == -32767); // last level, last number
    }
}

BOOST_AUTO_TEST_CASE( test_fillWriterConfig )
{
    const string fileName = pathTest("coordRefTimeTest.nc");
    const string inFillName = pathTest("fillIn3.nc");
    const string outFile = "fillOut.nc";
    copyFile(fileName, outFile);
    {
        CDMReader_p in = CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, inFillName);
        CDMReaderWriter_p out = CDMFileReaderFactory::createReaderWriter(MIFI_FILETYPE_NETCDF, outFile);
        FillWriter(in, out, pathTest("fillWriterConfig.xml"));
        // close the writer before re-reading it
    }
    {
        CDMReader_p out = CDMFileReaderFactory::create(MIFI_FILETYPE_NETCDF, outFile);
        BOOST_CHECK(out->getData("longitude")->asFloat()[2*11]+13 < 1e-5);
        BOOST_CHECK(out->getData("latitude")->asFloat()[2*11]-28 < 1e-5);
    }
}

#endif // HAVE_BOOST_UNIT_TEST_FRAMEWORK
