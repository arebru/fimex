/*
 * Fimex, AtmosphereSigma.h
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
 *  Created on: Aug 6, 2013
 *      Author: heikok
 */

#ifndef ATMOSPHERESIGMA_H_
#define ATMOSPHERESIGMA_H_

#include "fimex/coordSys/verticalTransform/VerticalTransformation.h"

namespace MetNoFimex
{
/**
 * @headerfile fimex/coordSys/verticalTransform/AtmosphereSigma.h
 */
/// Sigma pressure vertical coordinate
class AtmosphereSigma : public VerticalTransformation
{
public:
    const std::string sigma;
    const std::string ps;
    const std::string ptop;
    /**
     * Initialize AtmosphereSigma with formula
     *   p(k, x,y,t) = ptop + sigma(k)*(ps(x,y,t)-ptop)
     *
     * @param sigma  dimensionless parameter
     * @param ptop top of model
     * @param ps surface pressure
     */
    AtmosphereSigma(std::string sigma, std::string ptop, std::string ps) : sigma(sigma), ps(ps), ptop(ptop) {}
    virtual ~AtmosphereSigma() {}
    /**
     * static NAME constant
     * @return atmosphere_sigma_coordinate
     */
    static const std::string NAME() {return "atmosphere_hybrid_sigma_pressure_coordinate_2";}
    /**
     * @return same as static NAME()
     */
    virtual std::string getName() const { return NAME(); }
    virtual std::string getParameterString() const { return "sigma="+sigma+",ps="+ps+",ptop="+ptop; }
    virtual bool isComplete() const {return sigma != "" && ptop != "" && ps != "";}
protected:
    virtual boost::shared_ptr<ToVLevelConverter> getPressureConverter(const boost::shared_ptr<CDMReader>& reader, size_t unLimDimPos, boost::shared_ptr<const CoordinateSystem> cs, size_t nx, size_t ny, size_t nt) const;
};

} /* namespace MetNoFimex */
#endif /* ATMOSPHERESIGMA_H_ */