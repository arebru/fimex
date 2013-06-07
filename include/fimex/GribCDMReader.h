/*
 * Fimex, GribCDMReader.h
 *
 * (C) Copyright 2009, met.no
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
 *  Created on: Sep 9, 2009
 *      Author: Heiko Klein
 */

#ifndef GRIBCDMREADER_H_
#define GRIBCDMREADER_H_

#ifndef MIFI_IO_READER_SUPPRESS_DEPRECATED
#warning \
  This header-file is deprecated and \
  may be removed without further notice at a future date. Please use a \
  non-deprecated interface with equivalent functionality instead, i.e. \
  instead of \
    *CDMReader(file,config) \
  use \
    CDMFileReaderFactory::create(MIFI_FILETYPE_*,file,config)
#endif

#include <vector>
#include "boost/shared_ptr.hpp"
#include "fimex/GribFileIndex.h"
#include "fimex/CDMReader.h"
#include "fimex/ReplaceStringObject.h"
#include "fimex/XMLInput.h"
#include "fimex/XMLDoc.h"

namespace MetNoFimex
{

// forward decl.
class CDM;
class CDMDimension;
struct GribCDMReaderImpl;

class GribCDMReader: public MetNoFimex::CDMReader
{
public:
    GribCDMReader(const std::vector<std::string>& fileNames, const XMLInput& configXML, const std::vector<std::string>& members=std::vector<std::string>());
    virtual ~GribCDMReader();
    using CDMReader::getDataSlice;
    virtual DataPtr getDataSlice(const std::string& varName, size_t unLimDimPos);

private:
    // pimpl
    boost::shared_ptr<GribCDMReaderImpl> p_;

    /** Define which parameters to select
     * @param select can be "all", "definedOnly"
     */
    void initSelectParameters(const std::string& select);
    /**
     * find the node in the xml-config corresponding to the GribFileMessage
     * @return 0 if not found, otherwise a valid node
     */
    xmlNodePtr findVariableXMLNode(const GribFileMessage& msg) const;
    std::string getVariableName(const GribFileMessage& gfm) const;
    size_t getVariableMaxEnsembles(std::string varName) const;

    void initAddTimeDimension();
    void initAddGlobalAttributes();
    void initCreateGFIBoxes();
    void initLevels();

    /// key of the levelDimensions is gribEdition_levelType, i.e. 2_100
//    void initLevels(long edition, const std::map<long, std::set<long> >& levelsOfType, std::map<std::string, CDMDimension>& levelDimsOfType);
//    std::map<std::string, CDMDimension> initAddLevelDimensions();
    void initAddEnsembles();
    void initAddProjection();
    void initAddVariables();

    // read levels (pv) from a variable
    std::vector<double> readVarPv_(std::string exampleVar);
    std::vector<double> readValuesFromXPath_(xmlNodePtr node, DataPtr levelData, std::string exampleVar, std::string extension);
    void initSpecialLevels_(xmlNodePtr node, const std::string& extension, const std::string& levelType, std::size_t levelPos, const std::vector<std::string>& levelShape, DataPtr& levelData);
};


}

#endif /* GRIBCDMREADER_H_ */
