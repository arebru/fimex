#ifndef NETCDF_CDMWRITER_H_
#define NETCDF_CDMWRITER_H_

#include "CDMWriter.h"
#include "config.h"
#include <map>
#include <string>
#include NETCDF_CPP_INCLUDE

namespace MetNoFimex
{
class XMLDoc; // forward decl.

class NetCDF_CDMWriter : public CDMWriter
{
	typedef std::map<std::string, NcDim*> NcDimMap;
	typedef std::map<std::string, NcVar*> NcVarMap;

public:
	NetCDF_CDMWriter(const boost::shared_ptr<CDMReader> cdmReader, const std::string& outputFile);
	/**
	 * @param cdmReader dataSource
	 * @param outputFile file-name to write to
	 * @param configFile xml-configuration
	 */
	NetCDF_CDMWriter(const boost::shared_ptr<CDMReader> cdmReader, const std::string& outputFile, const std::string& configFile);
	virtual ~NetCDF_CDMWriter();
	/** @return the new name of a variable, eventually changed by the writers config */
	const std::string& getVariableName(const std::string& varName) const;
	/** @return the new name of a dimension, eventually changed by the writers config */
	const std::string& getDimensionName(const std::string& dimName) const;
	/** @return the new name of an attribute, eventually changed by the writers config */
	const std::string& getAttributeName(const std::string& varName, const std::string& attName) const;
	/** 
	 * @param varName original variable name  (before config: newname)
	 * @param attName original attribute name (before config: newname)
	 * @return an attribute contained in the writers attribute, possibly added by config 
	 */
	const CDMAttribute& getAttribute(const std::string& varName, const std::string& attName) const throw(CDMException);



private:
	NcError ncErr;
	NcFile ncFile;
	void init() throw(CDMException);
	void initFillRenameDimension(const std::auto_ptr<XMLDoc>& doc) throw(CDMException);
	void initFillRenameVariable(const std::auto_ptr<XMLDoc>& doc) throw(CDMException);
	void initFillRenameAttribute(const std::auto_ptr<XMLDoc>& doc) throw(CDMException);

	NcDimMap defineDimensions();
	NcVarMap defineVariables(const NcDimMap& dimMap);
	void writeAttributes(const NcVarMap& varMap);
	void writeData(const NcVarMap& varMap);
	std::map<std::string, std::string> variableNameChanges;
	std::map<std::string, CDMDataType> variableTypeChanges;
	std::map<std::string, std::string> dimensionNameChanges;
	std::map<std::string, std::map<std::string, std::string> > attributeNameChanges;
	CDM::StrStrAttrMap attributes;
};

}

#endif /*NETCDF_CDMWRITER_H_*/
