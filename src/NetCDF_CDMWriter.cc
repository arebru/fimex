/*
 * Fimex
 *
 * (C) Copyright 2008, met.no
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
 */

#include "fimex/NetCDF_CDMWriter.h"
#include "fimex_config.h"
#ifndef HAVE_NETCDF_HDF5_LIB
#undef NC_NETCDF4 /* netcdf4.1.2-4.2 define NC_NETCDF4 even when functions are not in library */
#endif
#ifdef HAVE_MPI
#include "fimex/mifi_mpi.h"
extern "C" {
#include "netcdf_par.h"
}
#else
extern "C" {
#include "netcdf.h"
}
#endif

#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <numeric>
#include <functional>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "fimex/mifi_constants.h"
#include "fimex/CDMDataType.h"
#include "fimex/DataTypeChanger.h"
#include "fimex/Units.h"
#include "fimex/Utils.h"
#include "fimex/XMLDoc.h"
#include "fimex/Logger.h"
#include "fimex/NcmlCDMReader.h"
#include "fimex/Data.h"
#include "NetCDF_Utils.h"

namespace MetNoFimex
{

namespace {

LoggerPtr logger = getLogger("fimex.NetCDF_CDMWriter");

int getNcVersion(int version, std::auto_ptr<XMLDoc>& doc)
{
    int retVal = NC_CLOBBER;
    switch (version) {
        case 3: retVal = NC_CLOBBER; break;
#ifdef NC_NETCDF4
        case 4: retVal = NC_CLOBBER | NC_CLASSIC_MODEL | NC_NETCDF4; break;
#endif
        default: LOG4FIMEX(logger, Logger::ERROR, "unknown netcdf-version "<< version << " using 3 instead"); break;
    }
    if (doc.get() != 0) {
        // set the default filetype
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/default[@filetype]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        if (nodes->nodeNr) {
            std::string filetype = string2lowerCase(getXmlProp(nodes->nodeTab[0], "filetype"));
            retVal = NC_CLOBBER;
            if (filetype == "netcdf3") {
                //retVal = NC_CLOBBER;
            } else if (filetype == "netcdf3_64bit") {
                retVal |= NC_64BIT_OFFSET;
            }
#ifdef NC_NETCDF4
            else if (filetype == "netcdf4") {
                retVal |= NC_NETCDF4;
            } else if (filetype == "netcdf4classic") {
                retVal |= NC_NETCDF4 | NC_CLASSIC_MODEL;
            }
#endif
            else {
                throw CDMException("unknown netcdf-filetype: " + filetype);
            }
        }
    }
    return retVal;
}

int ncDimId(int ncId, const CDMDimension* unLimDim)
{
    int unLimDimId = -1;
    if (unLimDim)
        ncCheck(nc_inq_dimid(ncId, unLimDim->getName().c_str(), &unLimDimId));
    return unLimDimId;
}

void checkDoc(std::auto_ptr<XMLDoc>& doc, const std::string& filename)
{
    XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    if (size == 0)
        throw CDMException("no root element (/cdm_ncwriter_config) in " + filename);
}

} // namespace

NetCDF_CDMWriter::NetCDF_CDMWriter(CDMReader_p cdmReader, const std::string& outputFile, std::string configFile, int version)
: CDMWriter(cdmReader, outputFile), ncFile(std::auto_ptr<Nc>(new Nc()))
{
    std::auto_ptr<XMLDoc> doc;
    if (configFile == "") {
        doc = std::auto_ptr<XMLDoc>(0);
    } else {
        doc = std::auto_ptr<XMLDoc>(new XMLDoc(configFile));
        checkDoc(doc, configFile);
    }
    int ncVersion = getNcVersion(version, doc);
    ncFile->filename = outputFile;
#ifdef HAVE_MPI
    if (mifi_mpi_initialized() && (mifi_mpi_size > 1)) {
        LOG4FIMEX(logger, Logger::DEBUG, "opening parallel nc-file: " << ncFile->filename);
        ncCheck(nc_create_par(ncFile->filename.c_str(), ncVersion | NC_MPIIO, mifi_mpi_comm, mifi_mpi_info, &ncFile->ncId), "creating "+ncFile->filename);
    } else {
        ncCheck(nc_create(ncFile->filename.c_str(), ncVersion, &ncFile->ncId), "creating "+ncFile->filename);
    }
#else
    ncCheck(nc_create(ncFile->filename.c_str(), ncVersion, &ncFile->ncId), "creating "+ncFile->filename);
#endif
    ncFile->isOpen = true;


    ncCheck(nc_inq_format(ncFile->ncId, &ncFile->format));
#ifdef NC_NETCDF4
    if ((ncFile->format == NC_FORMAT_NETCDF4) || (ncFile->format == NC_FORMAT_NETCDF4_CLASSIC)) {
        if ((ncVersion & NC_CLASSIC_MODEL) != 0) {
            LOG4FIMEX(logger, Logger::DEBUG, "netcdf4 format, classic mode");
        } else {
            LOG4FIMEX(logger, Logger::DEBUG, "netcdf4 format");
        }
    } else
#endif
    LOG4FIMEX(logger, Logger::DEBUG, "netcdf3 format");
    if (ncFile->format < 3) {
        /*
         *  using nofill for netcdf3 (2times io)
         *  doesn't effect netcdf4
         */
        int oldFill;
        nc_set_fill(ncFile->ncId, NC_NOFILL, &oldFill);
    }
    initNcmlReader(doc);
    initRemove(doc);
    // variable needs to be called before dimension!!!
    initFillRenameVariable(doc);
    initFillRenameDimension(doc);
    initFillRenameAttribute(doc);

    init();
}

void NetCDF_CDMWriter::initNcmlReader(std::auto_ptr<XMLDoc>& doc)
{
    if (doc.get() != 0) {
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/ncmlConfig");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        if (size > 0) {
            std::string configFile = getXmlProp(nodes->nodeTab[0], "filename");
            LOG4FIMEX(logger, Logger::DEBUG, "configuring CDMWriter with ncml config file: " << configFile);
            cdmReader = boost::make_shared<NcmlCDMReader>(cdmReader, XMLInputFile(configFile));
        }
    }
    cdm = cdmReader->getCDM();
}

void NetCDF_CDMWriter::initRemove(std::auto_ptr<XMLDoc>& doc)
{
    bool autoRemoveDims = true;
    if (doc.get() != 0) {
        {
            // remove global attributes
            XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/remove[@type='attribute']");
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            int size = (nodes) ? nodes->nodeNr : 0;
            for (int i = 0; i < size; i++) {
                std::string name = getXmlProp(nodes->nodeTab[i], "name");
                cdm.removeAttribute(CDM::globalAttributeNS(), name);
            }
        }
        {
            // remove variable attributes
            XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/variable/remove[@type='attribute']");
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            int size = (nodes) ? nodes->nodeNr : 0;
            for (int i = 0; i < size; i++) {
                std::string name = getXmlProp(nodes->nodeTab[i], "name");
                std::string varName = getXmlProp(nodes->nodeTab[i]->parent, "name");
                cdm.removeAttribute(varName, name);
            }
        }
        {
            // remove variables
            XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/remove[@type='variable']");
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            int size = (nodes) ? nodes->nodeNr : 0;
            for (int i = 0; i < size; i++) {
                std::string name = getXmlProp(nodes->nodeTab[i], "name");
                LOG4FIMEX(logger, Logger::DEBUG, "Removing variables '" << name << "'");
                cdm.removeVariable(name);
            }
        }
        {
            // remove dimensions
            XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/remove[@type='dimension']");
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            int size = (nodes) ? nodes->nodeNr : 0;
            for (int i = 0; i < size; i++) {
                std::string name = getXmlProp(nodes->nodeTab[i], "name");
                if (cdm.testDimensionInUse(name)) {
                    LOG4FIMEX(logger, Logger::WARN, "Cannot remove dimension in use: '" << name << "'");
                } else {
                    LOG4FIMEX(logger, Logger::DEBUG, "Removing dimension '" << name << "'");
                    cdm.removeDimension(name);
                }
            }
        }
        {
            // check for autoRemoveUnusedDimensions
            XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/default[@autoRemoveUnusedDimension='false']");
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            int size = (nodes) ? nodes->nodeNr : 0;
            if (size > 0) autoRemoveDims = false;
         }
    }
    if (autoRemoveDims) {
        const CDM::DimVec dims = cdm.getDimensions();
        for (CDM::DimVec::const_iterator dimIt = dims.begin(); dimIt != dims.end(); ++dimIt) {
            if (! cdm.testDimensionInUse(dimIt->getName())) {
                LOG4FIMEX(logger, Logger::DEBUG, "Auto-removing dimension '" << dimIt->getName() << "'");
                cdm.removeDimension(dimIt->getName());
            }
        }
    }
}

void NetCDF_CDMWriter::initFillRenameDimension(std::auto_ptr<XMLDoc>& doc)
{
    if (doc.get() != 0) {
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/dimension[@newname]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            LOG4FIMEX(logger, Logger::WARN, "Changing renaming dimensions in cdmWriterConfig.xml is deprecated, use ncmlCDMConfig instead!");
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            std::string newname = getXmlProp(nodes->nodeTab[i], "newname");
            cdm.getDimension(name); // check existence, throw exception
            dimensionNameChanges[name] = newname;
            // change dimension variable unless it has been changed
            if (variableNameChanges.find(name) == variableNameChanges.end()) {
                variableNameChanges[name] = newname;
            }
        }
    }
}

void NetCDF_CDMWriter::testVariableExists(const std::string& varName)
{
    try {
        cdm.getVariable(varName);
    } catch (CDMException& e) {
        throw CDMException(std::string("error modifying variable in writer: ") + e.what());
    }
}

void NetCDF_CDMWriter::initFillRenameVariable(std::auto_ptr<XMLDoc>& doc)
{
    if (doc.get() != 0) {
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/variable[@newname]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            LOG4FIMEX(logger, Logger::WARN, "Changing variable-names in cdmWriterConfig.xml is deprecated, use ncmlCDMConfig instead!");
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            testVariableExists(name);
            std::string newname = getXmlProp(nodes->nodeTab[i], "newname");
            variableNameChanges[name] = newname;
        }
    }
    if (doc.get() != 0) {
        // read 'type' attribute and enable re-typeing of data
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/variable[@type]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            CDMDataType type = string2datatype(getXmlProp(nodes->nodeTab[i], "type"));
            CDMVariable& v = cdm.getVariable(name);
            v.setDataType(type);
            variableTypeChanges[name] = type;
        }
    }
    unsigned int defaultCompression = 13; // shuffle + comp-level 3
    if (doc.get() != 0) {
        // set the default compression level for all variables
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/default[@compressionLevel]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        if (nodes->nodeNr) {
            defaultCompression = string2type<unsigned int>(getXmlProp(nodes->nodeTab[0], "compressionLevel"));
        }
    }
    const CDM::VarVec& vars = cdm.getVariables();
    for (CDM::VarVec::const_iterator varIt = vars.begin(); varIt != vars.end(); ++varIt) {
        variableCompression[varIt->getName()] = defaultCompression;
    }
    if (doc.get() != 0) {
        // set the compression level for all variables
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/variable[@compressionLevel]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            unsigned int compression = string2type<unsigned int>(getXmlProp(nodes->nodeTab[i], "compressionLevel"));
            variableCompression[name] = compression;
        }
    }
    // chunking
    if (doc.get() != 0) {
        // set the compression level for all variables
        XPathObjPtr xpathObj = doc->getXPathObject("/cdm_ncwriter_config/dimension[@chunkSize]");
        xmlNodeSetPtr nodes = xpathObj->nodesetval;
        int size = (nodes) ? nodes->nodeNr : 0;
        for (int i = 0; i < size; i++) {
            std::string name = getXmlProp(nodes->nodeTab[i], "name");
            unsigned int chunkSize = string2type<unsigned int>(getXmlProp(nodes->nodeTab[i], "chunkSize"));
            dimensionChunkSize[name] = chunkSize;
        }
    }
}

void NetCDF_CDMWriter::initFillRenameAttribute(std::auto_ptr<XMLDoc>& doc)
{
    // make a complete copy of the original attributes
    if (doc.get() != 0) {
    XPathObjPtr xpathObj = doc->getXPathObject("//attribute");
    xmlNodeSetPtr nodes = xpathObj->nodesetval;
    int size = (nodes) ? nodes->nodeNr : 0;
    for (int i = 0; i < size; i++) {
        xmlNodePtr node = nodes->nodeTab[i];
        std::string attName = getXmlProp(node, "name");
        if (attName != "units") LOG4FIMEX(logger, Logger::WARN, "Changing attributes in cdmWriterConfig.xml is deprecated, use ncmlCDMConfig instead!");
        std::string varName = CDM::globalAttributeNS();
        xmlNodePtr parent = node->parent;
        std::string parentName = getXmlName(parent);
        if (parentName == "cdm_ncwriter_config") {
            // default
        } else if (parentName == "variable") {
            varName = getXmlProp(parent, "name");
            testVariableExists(varName);
        } else {
            throw CDMException("unknown parent of attribute "+attName+": "+parentName);
        }

        std::string attValue = getXmlProp(node, "value");
        std::string attType = getXmlProp(node, "type");
        std::string attNewName = getXmlProp(node, "newname");
        std::string newAttrName = attNewName != "" ? attNewName : attName;
        CDMAttribute attr;
        if (attType != "") {
            attr = CDMAttribute(newAttrName, attType, attValue);
        } else {
            const CDMAttribute& oldAttr = cdm.getAttribute(varName, attName);
            attr = CDMAttribute(newAttrName, oldAttr.getDataType(), oldAttr.getData());
        }
        cdm.removeAttribute(varName, attName); // remove the attribute with the old name
        cdm.addAttribute(varName, attr); // set the attribute with the new name and data
    }
    }
}


NetCDF_CDMWriter::NcDimIdMap NetCDF_CDMWriter::defineDimensions() {
    const CDM::DimVec& cdmDims = cdm.getDimensions();
    NcDimIdMap ncDimMap;
    for (CDM::DimVec::const_iterator it = cdmDims.begin(); it != cdmDims.end(); ++it) {
#ifdef HAVE_MPI
        int length;
        if (mifi_mpi_initialized() && (mifi_mpi_size > 1)) {
            // netcdf-MPI does not work with unlimited variables
            length = it->getLength();
            if (length == 0) length++;
        } else {
            length = it->isUnlimited() ? NC_UNLIMITED : it->getLength();
        }
#else
        int length = it->isUnlimited() ? NC_UNLIMITED : it->getLength();
#endif
        if (!it->isUnlimited()) {
            // length = 0 means unlimited in netcdf, so I create a dummy
            if (length == 0) length = 1;
        }
        // NcDim is organized by NcFile, no need to clean
        // change the name written to the file according to getDimensionName
        int dimId;
        ncCheck(nc_def_dim(ncFile->ncId, getDimensionName(it->getName()).c_str(), length, &dimId));
        ncDimMap[it->getName()] = dimId;
        LOG4FIMEX(logger,Logger::DEBUG, "DimId of " << it->getName() << " = " << dimId);
    }
    return ncDimMap;
}

NetCDF_CDMWriter::NcVarIdMap NetCDF_CDMWriter::defineVariables(const NcDimIdMap& ncDimIdMap) {
    const CDM::VarVec& cdmVars = cdm.getVariables();
    NcVarIdMap ncVarMap;
    for (CDM::VarVec::const_iterator it = cdmVars.begin(); it != cdmVars.end(); ++it) {
        const CDMVariable& var = *it;
        const std::vector<std::string>& shape = var.getShape();
        boost::scoped_array<int> ncshape(new int[shape.size()]);
        for (size_t i = 0; i < shape.size(); i++) {
            // revert order, cdm requires fastest moving first, netcdf-cplusplus requires fastest moving first
            ncshape[i] = ncDimIdMap.find(shape[(shape.size()-1-i)])->second;
        }
        CDMDataType datatype = var.getDataType();
        if (variableTypeChanges.find(var.getName()) != variableTypeChanges.end()) {
            CDMDataType& newType = variableTypeChanges[var.getName()];
            datatype = newType != CDM_NAT ? newType : datatype;
        }
        if (datatype == CDM_NAT && shape.size() == 0) {
            // empty variable, use int datatype
            datatype = CDM_INT;
        }
        int varId;
        LOG4FIMEX(logger, Logger::DEBUG, "defining variable " << var.getName() << " with shape '" << join(shape.begin(), shape.end()) << "' = " <<join(&ncshape[0], &ncshape[0]+shape.size()));
        ncCheck(nc_def_var(ncFile->ncId, getVariableName(var.getName()).c_str(), cdmDataType2ncType(datatype), shape.size(), &ncshape[0], &varId));
        ncVarMap[var.getName()] = varId;
#ifdef NC_NETCDF4
        // set compression
        if ((ncFile->format == NC_FORMAT_NETCDF4) || (ncFile->format == NC_FORMAT_NETCDF4_CLASSIC)) {
#ifdef HAVE_MPI
            if (mifi_mpi_initialized() && (mifi_mpi_size > 1)) {
                LOG4FIMEX(logger, Logger::INFO, "disabling all compression, not possible with parallel HDF5");
            } else {
#endif //HAVE_MPI
            int compression = 0;
            int shuffle = 0;
            assert(variableCompression.find(var.getName()) != variableCompression.end());
            if (variableCompression.find(var.getName()) != variableCompression.end()) {
                compression = variableCompression[var.getName()];
                if (compression > 10) {
                    compression -= 10;
                    shuffle = 1;
                }
            }
            if (compression > 0 &&  shape.size() >= 1) { // non-scalar variables
                // create a chunk-strategy: continuous in last dimensions, max MAX_CHUNK
                const size_t DEFAULT_CHUNK = 2 << 20; // good chunk up to 1M *sizeof(type)
                const size_t MIN_CHUNK = 2 << 16; // chunks should be at least reasonably sized, e.g. 64k*sizeof(type)
                boost::scoped_array<size_t> ncChunk(new size_t[shape.size()]);
                size_t chunkSize = 1;
                for (size_t i = 0; i < shape.size(); i++) {
                    // revert order, cdm requires fastest moving first, netcdf-c requires fastest moving last
                    CDMDimension& dim = cdm.getDimension(shape.at(i));
                    size_t dimSize = dim.isUnlimited() ? 1 : dim.getLength();
                    std::map<std::string, unsigned int>::const_iterator defaultChunk = dimensionChunkSize.find(shape.at(i));
                    if (defaultChunk != dimensionChunkSize.end()) {
                        unsigned int chunkDim = (defaultChunk->second > dimSize) ? dimSize : defaultChunk->second;
                        if (chunkDim == 0) chunkDim = 1;
                        ncChunk[shape.size()-1 - i] = chunkDim;
                        chunkSize *= chunkDim;
                    } else {
                        size_t lastChunkSize = chunkSize;
                        chunkSize *= dimSize;
                        if (chunkSize < DEFAULT_CHUNK) {
                            ncChunk[shape.size()-1 - i] = dimSize;
                        } else {
                            size_t thisChunk = 1;
                            if (dimSize > 1 && (lastChunkSize < (MIN_CHUNK))) {
                                // create a chunk-size which makes the total chunk ~= MIN_CHUNK
                                thisChunk = floor(chunkSize / MIN_CHUNK); // a number > 2^4 since chunkSize > DEFAULT_CHUNK
                                if (thisChunk > dimSize) thisChunk = dimSize;
                                if (thisChunk < 1) thisChunk = 1;
                            }
                            ncChunk[shape.size()-1 - i] = thisChunk;
                        }
                    }
                }
                if (chunkSize > 0) {
                    LOG4FIMEX(logger, Logger::DEBUG, "chunk variable " << var.getName() << " to " << join(&ncChunk[0], &ncChunk[0] + shape.size(), "x"));
                    ncCheck(nc_def_var_chunking(ncFile->ncId, varId, NC_CHUNKED, ncChunk.get()));
                }
                // start compression
                LOG4FIMEX(logger, Logger::DEBUG, "compressing variable " << var.getName() << " with level " << compression << " and shuffle="<< shuffle);
                ncCheck(nc_def_var_deflate(ncFile->ncId, varId, shuffle, 1, compression));
            }
#ifdef HAVE_MPI
            }
#endif // HAVE_MPI
        }
#endif // NC_NETCDF4
    }
    return ncVarMap;
}

void NetCDF_CDMWriter::writeAttributes(const NcVarIdMap& ncVarMap) {
    // using C interface since it offers a combined interface to global and var attributes
    const CDM::StrAttrVecMap& attributes = cdm.getAttributes();
    for (CDM::StrAttrVecMap::const_iterator it = attributes.begin(); it != attributes.end(); ++it) {
        int varId;
        if (it->first == CDM::globalAttributeNS()) {
            varId = NC_GLOBAL;
        } else {
            varId = ncVarMap.find(it->first)->second;
        }
        for (CDM::AttrVec::const_iterator ait = it->second.begin(); ait != it->second.end(); ++ait) {
            const CDMAttribute& attr = *ait;
            CDMDataType dt = attr.getDataType();
            switch (dt) {
            case CDM_STRING: ;
                ncCheck(nc_put_att_text(ncFile->ncId, varId, attr.getName().c_str(), attr.getData()->size(), attr.getData()->asChar().get() ));
                break;
            case CDM_CHAR:
                ncCheck(nc_put_att_schar(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), reinterpret_cast<const signed char*>(attr.getData()->asChar().get()) ));
                break;
            case CDM_SHORT:
                ncCheck(nc_put_att_short(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asShort().get() ));
                break;
            case CDM_INT:
                ncCheck(nc_put_att_int(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asInt().get() ));
                break;
            case CDM_FLOAT:
                ncCheck(nc_put_att_float(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asFloat().get() ));
                break;
            case CDM_DOUBLE:
                ncCheck(nc_put_att_double(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asDouble().get() ));
                break;
#ifdef NC_NETCDF4
            case CDM_STRINGS: {
                DataPtr attrData = attr.getData();
                const size_t attrLen = attrData->size();
                boost::shared_array<std::string> svals = attrData->asStrings();
                if (ncFile->supports_nc_string()) {
                    boost::shared_array<const char*> cvals(new const char*[attrLen]);
                    for (size_t i=0; i<attrLen; ++i)
                        cvals[i] = svals[i].c_str();
                    ncCheck(nc_put_att(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attrLen, &cvals[0]));
                } else if (attrLen == 1) {
                    // convert 1 string to char attribute
                    const std::string& s0 = svals[0];
                    ncCheck(nc_put_att_text(ncFile->ncId, varId, attr.getName().c_str(), s0.size(), s0.c_str()));
                } else {
                    throw CDMException("cannot write CDM_STRINGS with more than 1 entry to this NetCDF format for attribute '" + attr.getName() + "'");
                }
                break;
            }
            case CDM_UCHAR:
                ncCheck(nc_put_att_uchar(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asUChar().get() ));
                break;
            case CDM_USHORT:
                ncCheck(nc_put_att_ushort(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asUShort().get() ));
                break;
            case CDM_UINT:
                ncCheck(nc_put_att_uint(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asUInt().get() ));
                break;
            case CDM_INT64:
                ncCheck(nc_put_att_longlong(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asInt64().get() ));
                break;
            case CDM_UINT64:
                ncCheck(nc_put_att_ulonglong(ncFile->ncId, varId, attr.getName().c_str(), cdmDataType2ncType(dt), attr.getData()->size(), attr.getData()->asUInt64().get() ));
                break;
#endif
            case CDM_NAT:
            default: throw CDMException("unknown datatype for attribute " + attr.getName());
            }
        }
    }
}

double NetCDF_CDMWriter::getOldAttribute(const std::string& varName, const std::string& attName, double defaultValue) const
{
    double retVal = defaultValue;
    try {
        const CDMAttribute& attr = cdmReader->getCDM().getAttribute(varName, attName);
        retVal = attr.getData()->asDouble()[0];
    } catch (CDMException& e) {} // don't care
    return retVal;
}
double NetCDF_CDMWriter::getNewAttribute(const std::string& varName, const std::string& attName, double defaultValue) const
{
    double retVal = defaultValue;
    try {
        const CDMAttribute& attr = getAttribute(varName, attName);
        retVal = attr.getData()->asDouble()[0];
    } catch (CDMException& e) {} // don't care
    return retVal;
}


void NetCDF_CDMWriter::writeData(const NcVarIdMap& ncVarMap) {
    const CDMDimension* unLimDim = cdm.getUnlimitedDim();
    const int unLimDimId = ncDimId(ncFile->ncId, unLimDim);
    const long long maxUnLim = (unLimDim == 0) ? 0 : unLimDim->getLength();
    const CDM::VarVec& cdmVars = cdm.getVariables();
    // write data
#if !defined(__INTEL_COMPILER) // openmp gives currently segfaults with intel compilers
#ifdef _OPENMP
#pragma omp parallel default(none) shared(logger, cdmVars, ncVarMap)
    {
#pragma omp single
        {
#endif
#endif //__INTEL_COMPILER
            Units units;

            // read data along unLimDim and then variables, otherwise netcdf3 reading might get very slow
            // see http://www.unidata.ucar.edu/support/help/MailArchives/netcdf/msg10905.html
            // use unLimDimPos = -1 for variables without unlimited dimension
            for (long long unLimDimPos = -1; unLimDimPos < maxUnLim; ++unLimDimPos) {
#ifdef HAVE_MPI
                const bool sliceAlongUnlimited = (maxUnLim > 3);
                if (mifi_mpi_initialized() && (mifi_mpi_size > 1)) {
                    if (sliceAlongUnlimited) { // MPI-slices along unlimited dimension
                        // only work on variables which belong to this mpi-process (modulo-base)
                        if ((unLimDimPos % mifi_mpi_size) != mifi_mpi_rank) {
                            LOG4FIMEX(logger, Logger::DEBUG, "processor " << mifi_mpi_rank << " skipping on unLimDimPos " << unLimDimPos);
                            continue;
                        } else {
                            LOG4FIMEX(logger, Logger::DEBUG, "processor " << mifi_mpi_rank << " working on unLimDimPos " << unLimDimPos);
                        }
                    }
                }
#endif
                for (size_t vi = 0; vi < cdmVars.size(); ++vi) {
                    CDMVariable cdmVar = cdmVars.at(vi);
                    std::string varName = cdmVar.getName();
                    int varId = ncVarMap.find(cdmVar.getName())->second;
#ifdef HAVE_MPI
                    if (mifi_mpi_initialized() && (mifi_mpi_size > 1)) {
                        ncCheck(nc_var_par_access(ncFile->ncId, varId, NC_INDEPENDENT));
                        if (!sliceAlongUnlimited) { // MPI-slices along unlimited dimension
                            // only work on variables which belong to this mpi-process (modulo-base along variable-ids)
                            if ((vi % mifi_mpi_size) != mifi_mpi_rank) {
                                LOG4FIMEX(logger, Logger::DEBUG, "processor " << mifi_mpi_rank << " skipping on variable " << varName);
                                continue;
                            } else {
                                LOG4FIMEX(logger, Logger::DEBUG, "processor " << mifi_mpi_rank << " working on variable '" << varName << "'");
                            }
                        }
                    }
#endif
                    DataTypeChanger dtc(cdmVar.getDataType());
                    if ((variableTypeChanges.find(varName) != variableTypeChanges.end()) &&
                            (variableTypeChanges[varName] != CDM_NAT)) {
                        double oldFill = cdmReader->getCDM().getFillValue(varName);
                        double oldScale = cdmReader->getCDM().getScaleFactor(varName);
                        double oldOffset = cdmReader->getCDM().getAddOffset(varName);
                        double newFill = cdm.getFillValue(varName);
                        double newScale = cdm.getScaleFactor(varName);
                        double newOffset = cdm.getAddOffset(varName);

                        // changes of the units
                        double unitSlope = 1.;
                        double unitOffset = 0.;
                        std::string oldUnit;
                        std::string newUnit;
                        try {
                            oldUnit = cdmReader->getCDM().getUnits(varName);
                            newUnit = cdm.getUnits(varName);
                            if (oldUnit != newUnit) {
                                units.convert(oldUnit, newUnit, unitSlope, unitOffset);
                            }
                        } catch (UnitException& e) {
                            LOG4FIMEX(logger, Logger::WARN, "unable to convert data-units for variable " << cdmVar.getName() << ": " << e.what());
                        } catch (CDMException& e) {
                            // units not defined, do nothing
                        }

                        dtc = DataTypeChanger(cdmVar.getDataType(), oldFill, oldScale, oldOffset, variableTypeChanges[cdmVar.getName()], newFill, newScale, newOffset, unitSlope, unitOffset);
                    }
                    int dimLen;
                    ncCheck(nc_inq_varndims(ncFile->ncId, varId, &dimLen));
                    int dimIds[dimLen];
                    ncCheck(nc_inq_vardimid(ncFile->ncId, varId, &dimIds[0]));
                    size_t count[dimLen];
                    size_t start[dimLen];
                    int unLimDimIdx = -1;
                    for (int i = 0; i < dimLen; ++i) {
                        start[i] = 0;
                        ncCheck(nc_inq_dimlen(ncFile->ncId, dimIds[i], &count[i]));
                        if (dimIds[i] == unLimDimId)
                            unLimDimIdx = i;
                    }
                    LOG4FIMEX(logger, Logger::DEBUG, "dimids of " << varName << ": " << join(&dimIds[0], &dimIds[0]+dimLen));
                    if (unLimDimPos == -1) {
                        if (!cdm.hasUnlimitedDim(cdmVar)) {
                            //variable without unlimited dimension write at -1
#if !defined(__INTEL_COMPILER)
#ifdef _OPENMP
#pragma omp task firstprivate(cdmVar,varName,vi)
                            {
#endif
#endif
                                DataPtr data = cdmReader->getData(varName);
                                try {
                                    data = dtc.convertData(data);
                                } catch (CDMException& e) {
                                    throw CDMException("problems converting data of var " + varName + ": " + e.what());
                                }
                                if (data->size() == 0 && ncFile->format < 3) {
                                    // need to write data with _FillValue,
                                    // since we are using NC_NOFILL for nc3 format files = NC_FORMAT_CLASSIC(1) NC_FORMAT_64BIT(2))
                                    size_t size = (dimLen > 0) ? std::accumulate(count, count + dimLen, 1, std::multiplies<size_t>()) : 1;
                                    data = createData(cdmVar.getDataType(), size, cdm.getFillValue(varName));
                                }

                                if (data->size() > 0) {
                                    try {
                                        ScopedCritical ncLock(Nc::getMutex());
                                        LOG4FIMEX(logger, Logger::DEBUG, "writing variable " << varName);
                                        ncPutValues(data, ncFile->ncId, varId, cdmDataType2ncType(cdmVar.getDataType()), dimLen, start, count);
                                    } catch (CDMException& ex) {
                                        throw CDMException(ex.what() + std::string(" while writing var ")+ varName );
                                    }
                                }
                            }
#if !defined(__INTEL_COMPILER)
#ifdef _OPENMP
                        }
#endif
#endif
                    } else {
                        if (cdm.hasUnlimitedDim(cdmVar) && unLimDimIdx >= 0) {
                            // variables with unlimited dimension
#if !defined(__INTEL_COMPILER)
#ifdef _OPENMP
#pragma omp task firstprivate(cdmVar,varName,vi,unLimDimPos)
                            {
#endif
#endif
                                DataPtr data = cdmReader->getDataSlice(cdmVar.getName(), unLimDimPos);
                                try {
                                    data = dtc.convertData(data);
                                } catch (CDMException& e) {
                                    throw CDMException("problems converting data of var " + cdmVar.getName() + ": " + e.what());
                                }
                                if (data->size() == 0 && ncFile->format < 3) {
                                    // need to write data with _FillValue,
                                    // since we are using NC_NOFILL for nc3 format files = NC_FORMAT_CLASSIC(1) NC_FORMAT_64BIT(2))
                                    count[0] = 1; // just one slice
                                    size_t size = (dimLen == 0) ? 1 : std::accumulate(count, count + dimLen, 1, std::multiplies<size_t>());
                                    data = createData(cdmVar.getDataType(), size, cdm.getFillValue(varName));
                                }
                                if (data->size() > 0) {
                                    count[unLimDimIdx] = 1;
                                    start[unLimDimIdx] = unLimDimPos;
                                    try {
                                        ScopedCritical ncLock(Nc::getMutex());
                                        LOG4FIMEX(logger, Logger::DEBUG, "writing variable " << varName << "("<< unLimDimPos << ")");
                                        ncPutValues(data, ncFile->ncId, varId, cdmDataType2ncType(cdmVar.getDataType()), dimLen, start, count);
                                    } catch (CDMException& ex) {
                                        throw CDMException(ex.what() + std::string(" while writing slice of var ")+ varName );
                                    }

                                }
#if !defined(__INTEL_COMPILER)
#ifdef _OPENMP
                            }
#endif
#endif
                        }
                    }
                }
#ifndef HAVE_MPI
                if (unLimDimPos >= 0) {
                    ScopedCritical ncLock(Nc::getMutex());
                    ncCheck(nc_sync(ncFile->ncId)); // sync every 'time/unlimited' step (does not work with MPI)
                }
#endif
            }
#if !defined(__INTEL_COMPILER)
#ifdef _OPENMP
        } // single
    } // parallel
#endif
#endif
}

void NetCDF_CDMWriter::init()
{
    // write metadata
    NcDimIdMap ncDimIdMap = defineDimensions();
    NcVarIdMap ncVarIdMap = defineVariables(ncDimIdMap);
    writeAttributes(ncVarIdMap);
    // write data
    ncCheck(nc_enddef(ncFile->ncId));
    writeData(ncVarIdMap);
}

NetCDF_CDMWriter::~NetCDF_CDMWriter()
{
}

const CDMAttribute& NetCDF_CDMWriter::getAttribute(const std::string& varName, const std::string& attName) const
{
    return cdm.getAttribute(varName, attName);
}

const std::string& NetCDF_CDMWriter::getVariableName(const std::string& varName) const
{
    if (variableNameChanges.find(varName) == variableNameChanges.end()) {
        return varName;
    } else {
        return variableNameChanges.find(varName)->second;
    }
}
const std::string& NetCDF_CDMWriter::getDimensionName(const std::string& dimName) const
{
    if (dimensionNameChanges.find(dimName) == dimensionNameChanges.end()) {
        return dimName;
    } else {
        return dimensionNameChanges.find(dimName)->second;
    }
}

}
