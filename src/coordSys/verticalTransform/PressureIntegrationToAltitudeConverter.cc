#include "fimex/coordSys/verticalTransform/PressureIntegrationToAltitudeConverter.h"

#include "fimex/CDM.h"
#include "fimex/CDMReader.h"
#include "fimex/CDMReaderUtils.h"
#include "fimex/coordSys/CoordinateSystem.h"
#include "fimex/coordSys/verticalTransform/VerticalTransformationUtils.h"
#include "fimex/Data.h"
#include "fimex/interpolation.h"
#include "fimex/Logger.h"
#include "fimex/vertical_coordinate_transformations.h"

#include "fimex/ArrayLoop.h"

#include <boost/make_shared.hpp>

namespace MetNoFimex {

static LoggerPtr logger = getLogger("fimex.PressureIntegrationToAltitudeConverter");

// static method
VerticalConverterPtr PressureIntegrationToAltitudeConverter::createConverter(CDMReader_p reader,
        CoordSysPtr cs, VerticalConverterPtr pressure)
{
    const CDM& rcdm = reader->getCDM();

    const std::vector<std::string> pshape = pressure->getShape();

    // air_temperature and/or pressureConverter define shape
    const std::string air_temperature = findVariableWithDims(rcdm, "air_temperature", pshape);
    if (air_temperature.empty()) {
        LOG4FIMEX(logger, Logger::DEBUG, "no air temperature");
        return VerticalConverterPtr();
    }

    const std::string specific_humidity = findVariableWithDims(rcdm, "specific_humidity", pshape); // no problem if empty
    std::vector<std::string> surface_shape(reader->getCDM().getVariable(air_temperature).getShape());
    if (CoordinateSystem::ConstAxisPtr zax = cs->getGeoZAxis())
        erase_value(surface_shape, zax->getName());
    removeDimsWithLength1(rcdm, surface_shape);

    const std::string surface_air_pressure = findVariableWithDims(rcdm, "surface_air_pressure", surface_shape);

    if (CoordinateSystem::ConstAxisPtr tax = cs->getTimeAxis())
        erase_value(surface_shape, tax->getName());
    const std::string surface_geopotential = findVariableWithDims(rcdm, "surface_geopotential", surface_shape);

    if (surface_air_pressure.empty() || surface_geopotential.empty()) {
        LOG4FIMEX(logger, Logger::DEBUG, "no surface_air_pressure or surface_geopotential");
        return VerticalConverterPtr();
    }
    return boost::make_shared<PressureIntegrationToAltitudeConverter>(reader, cs, pressure,
        air_temperature, specific_humidity, surface_air_pressure, surface_geopotential);
}


PressureIntegrationToAltitudeConverter::PressureIntegrationToAltitudeConverter(CDMReader_p reader, CoordSysPtr cs, VerticalConverterPtr pressure,
    const std::string& air_temperature, const std::string& specific_humidity, const std::string& surface_air_pressure, const std::string& surface_geopotential)
    : BasicVerticalConverter(reader, cs)
    , pressure_(pressure)
    , air_temperature_(air_temperature)
    , specific_humidity_(specific_humidity)
    , surface_air_pressure_(surface_air_pressure)
    , surface_geopotential_(surface_geopotential)
{
    LOG4FIMEX(logger, Logger::INFO, "using hypsometric equation with surface pressure '"
            << surface_air_pressure_ << "', surface geopotential '" << surface_geopotential_
            << "', air_temperature '" << air_temperature_ << "' to retrieve altitude");
    if (!specific_humidity_.empty())
        LOG4FIMEX(logger, Logger::INFO, "hypsometric equation uses virtual "
            "temperature with specific humidity '" << specific_humidity_ << "'");
}

std::vector<std::string> PressureIntegrationToAltitudeConverter::getShape() const
{
    return ShapeMerger(reader_->getCDM(), cs_)
            .merge(surface_air_pressure_, true)
            .merge(surface_geopotential_, true)
            .merge(air_temperature_)
            .merge(specific_humidity_) // empty specific_humidity_ handled in ShapeMerger
            .merge(pressure_)
            .shape();
}

DataPtr PressureIntegrationToAltitudeConverter::getDataSlice(const SliceBuilder& sbOut) const
{
    // we always need the whole pressure column below! -- difficult to integrate otherwise
    // 1) find vertical axis and its length
    // 2) read one full pressure column from pressure_ to find out if pressure is highest at start or end of vertical axis
    // 3) extend start or end of vertical dim in sb -> sbC; print a WARN about efficiency
    // 4) integrate from surface to end of requested range
    // 5) only store if inside requested range

    const CDM& rcdm = reader_->getCDM();
    // 1)
    const std::string& zName = rcdm.getVariable(cs_->getGeoZAxis()->getName()).getShape().front();
    const size_t n_z = rcdm.getDimension(zName).getLength();
    size_t sb_out_z_start, sb_out_z_size;
    sbOut.getStartAndSize(zName, sb_out_z_start, sb_out_z_size);
    const size_t sb_out_z_end = sb_out_z_start + sb_out_z_size;

    // TODO part of the following is only necessary if the request is for a part of the vertical axis

    // 2) TODO this can be cached
    SliceBuilder sbPressColumn = adaptSliceBuilder(rcdm, pressure_, sbOut);
    const std::vector<std::string> sbPressDims = sbPressColumn.getDimensionNames();
    for (size_t i = 0; i < sbPressDims.size(); ++i) {
        if (sbPressDims[i] == zName)
            sbPressColumn.setAll(zName);
        else
            sbPressColumn.setStartAndSize(sbPressDims[i], 0, 1);
    }
    DataPtr pressColumnData = pressure_->getDataSlice(sbPressColumn);
    assert(pressColumnData->size() == n_z);
    boost::shared_array<float> pColumnVal = pressColumnData->asFloat();
    const ptrdiff_t z_last = int(n_z)-1;
    assert(z_last > 0);
    const float p_first = pColumnVal[0];
    const float p_last = pColumnVal[z_last];
    assert(p_first > 0 && p_last > 0);
    // FIXME this does not work if the column we chose has undefined values
    const bool start_high_p = (p_first > p_last);

    // 3)
    int iz_step, iz_in_0, iz_out_0;
    size_t sb_z_begin, sb_z_size;
    if (start_high_p) {
        sb_z_begin = 0;
        sb_z_size = sb_out_z_end - sb_z_begin;
        iz_step = 1;               // increasing l => increasing altitude
        iz_in_0 = 0;               // start on surface
        iz_out_0 = -sb_out_z_start;
    } else {
        sb_z_begin = sb_out_z_start;
        sb_z_size = n_z - sb_z_begin;
        iz_step = -1;              // decreasing l => increasing altitude
        iz_in_0 = sb_z_size - 1;   // start on surface
        iz_out_0 = iz_in_0;
    }
    SliceBuilder sb = sbOut;
    sb.setStartAndSize(zName, sb_z_begin, sb_z_size);

    VarFloat sap(reader_, surface_air_pressure_, "hPa", sb);
    VarFloat sgp(reader_, surface_geopotential_, "m^2/s^2", sb);
    VarFloat airt(reader_, air_temperature_, "K", sb);
    VarFloat pressure(reader_, pressure_, sb);

    if (!sap.data || !sgp.data || !airt.data || !pressure.data) {
        LOG4FIMEX(logger, Logger::INFO, "hypsometric no data");
        return DataPtr();
    }

    ArrayDims soAltitude = makeArrayDims(sbOut);

    set_not_shared(zName, sap.dims, sgp.dims, airt.dims, pressure.dims, soAltitude);

    enum { IN_SAP, IN_SGP, IN_AIRT, IN_PRESSURE, OUT_ALTITUDE, IN_SHUM };
    ArrayGroup group;
    group.add(sap.dims).add(sgp.dims).add(airt.dims).add(pressure.dims).add(soAltitude);

    ArrayDims siSHum;
    boost::shared_array<float> shVal;
    if (!specific_humidity_.empty()) {
        SliceBuilder sbSHum = adaptSliceBuilder(rcdm, specific_humidity_, sb);
        if (DataPtr shData = getSliceData(reader_, sbSHum, specific_humidity_, "1")) {
            shVal = shData->asFloat();
            siSHum = makeArrayDims(sbSHum);
            set_not_shared(zName, siSHum);
            group.add(siSHum);
        }
    }

    group.minimizeShared(0); // FIXME do not treat each value separately

    const size_t dZPressure = pressure.dims.delta(zName), dZAirT = airt.dims.delta(zName);
    const size_t dZSHum = siSHum.delta(zName), dZAlti = soAltitude.delta(zName);

    const size_t size = soAltitude.volume();
    boost::shared_array<double> altiVal(new double[size]);

    // 4)
#if 0 && defined(_OPENMP)
#pragma omp parallel default(shared)
#endif
    Loop loop(group);
    do { // sharedVolume() == 1 because we called minimizeShared before
        double a = sgp.values[loop[IN_SGP]] / MIFI_EARTH_GRAVITY;
        float p_low_alti = sap.values[loop[IN_SAP]];
        int iz_in = iz_in_0, iz_out = iz_out_0;
        for (size_t i = 0; i < sb_z_size; i += 1, iz_in += iz_step, iz_out += iz_step) {
            const float p_high_alti = pressure.values[loop[IN_PRESSURE] + iz_in*dZPressure];

            float Tv = airt.values[loop[IN_AIRT] + iz_in*dZAirT];
            if (shVal) {
                Tv = mifi_virtual_temperature(shVal[loop[IN_SHUM] + iz_in*dZSHum], Tv);
            }

            const double lt = mifi_barometric_layer_thickness(p_low_alti, p_high_alti, Tv);
            a += lt;
            // 5)
            if (iz_out >= 0 && iz_out < sb_out_z_size) {
                LOG4FIMEX(logger, Logger::DEBUG, "storing alti=" << a);
                altiVal[loop[OUT_ALTITUDE] + iz_out*dZAlti] = a;
            }

            p_low_alti = p_high_alti;
        }
    } while (loop.next());
    return createData(size, altiVal);
}

} // namespace MetNoFimex
