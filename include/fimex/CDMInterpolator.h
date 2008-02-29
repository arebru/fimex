#ifndef CDMINTERPOLATOR_H_
#define CDMINTERPOLATOR_H_

#include "CDMReader.h"
#include "CachedInterpolation.h"

namespace MetNoUtplukk
{

class CDMInterpolator : public MetNoUtplukk::CDMReader
{
private: 
	boost::shared_ptr<CDMReader> dataReader;
	std::vector<std::string> projectionVariables;
	CachedInterpolation cachedInterpolation;
public:
	CDMInterpolator(boost::shared_ptr<CDMReader> dataReader);
	virtual ~CDMInterpolator();
	virtual const boost::shared_ptr<Data> getDataSlice(const CDMVariable& variable, size_t unLimDimPos = 0) throw(CDMException);
	/**
	 * @ brief change the (main) projection of the dataReaders cdm to this new projection
	 * 
	 * @param method Interpolation method 
	 */
	virtual void changeProjection(int method, const string& proj_input, const vector<double>& out_x_axis, const vector<double>& out_y_axis, const string& out_x_axis_unit, const string& out_y_axis_unit) throw(CDMException);
	
};

}

#endif /*CDMINTERPOLATOR_H_*/
