/*
 * Fimex, CDMPressureConversions.h
 *
 * (C) Copyright 2011, met.no
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
 *  Created on: Aug 12, 2011
 *      Author: Heiko Klein
 */

#ifndef CDMPRESSURECONVERSIONS_H_
#define CDMPRESSURECONVERSIONS_H_

/**
 * @headerfile "fimex/CDMVerticalInterpolator.h"
 */

#include "fimex/CDMReader.h"

namespace MetNoFimex
{

// forward declaration
struct CDMPressureConversionsImpl;

/**
 * CDMReader to convert pressure related variables, i.e. Theta or pressure
 * to other fields.
 */
class CDMPressureConversions: public MetNoFimex::CDMReader
{
public:
    /**
     * initialization with another dataReader
     * @param dataReader source of data
     * @param operations list of operations
     *    - theta2T translates theta (detected by standard_name) to air_temperature,
     *    - add4Dpressure will add variable pressure(t,k,x,y) comparable to the first 4D field found
     *
     * @warning the routine does not handle invalid values, except float/double nans
     */
    CDMPressureConversions(boost::shared_ptr<CDMReader> dataReader, std::vector<std::string> operations);
    virtual ~CDMPressureConversions() {}
    virtual boost::shared_ptr<Data> getDataSlice(const std::string& varName, size_t unLimDimPos = 0);
private:
    boost::shared_ptr<CDMReader> dataReader_;
    boost::shared_ptr<CDMPressureConversionsImpl> p_;
};

}

#endif /* CDMPRESSURECONVERSIONS_H_ */