/*
 * Fimex, GribFileIndex.h
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
 *  Created on: Aug 31, 2009
 *      Author: Heiko Klein
 */

#ifndef GRIBFILEINDEX_H_
#define GRIBFILEINDEX_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/operations.hpp>
#include <vector>
#include "fimex/XMLDoc.h"
#include "fimex/GridDefinition.h"
#include <boost/date_time/posix_time/posix_time_types.hpp>

// forward decl of grib_api
struct grib_handle;

namespace MetNoFimex
{
class GribFileMessage
{
public:
    GribFileMessage();
    GribFileMessage(boost::shared_ptr<grib_handle> gh, const std::string& fileURL, long filePos, long msgPos, const std::vector<std::string>& members=std::vector<std::string>());
    GribFileMessage(boost::shared_ptr<XMLDoc>, std::string nsPrefix, xmlNodePtr node);
    ~GribFileMessage();

    /// test if this is a proper GribFileMessage or just the default constructor
    bool isValid() const {return fileURL_ != "";}
    /// give a xml-string representation
    std::string toString() const;
    /// accessors
    const long getEdition() const;
    const std::string& getFileURL() const;
    const size_t getFilePosition() const;
    /// messages number within a multi-message
    const size_t getMessageNumber() const;
    const std::string& getName() const;
    const std::string& getShortName() const;
    boost::posix_time::ptime getValidTime() const;
    boost::posix_time::ptime getReferenceTime() const;
    /// return gribs timeRangeIndicator (0=instant, 2,4=accumulated)
    long getTimeRangeIndicator() const;
    long getLevelNumber() const;
    long getLevelType() const;
    /**
     * give the total number of ensembles for this parameter
     */
    size_t getTotalNumberOfEnsembles() const { return static_cast<size_t>(totalNumberOfEnsembles_); }
    /**
     * get the current perturbation/ensemble number
     */
    size_t getPerturbationNumber() const { return static_cast<size_t>(perturbationNo_); }
    /**
     * Get the parameter ids as list with the following meanings:
     * @li ed1: indicatorOfParameter, gribTablesVersionNo, identificationOfOriginatingGeneratingCentre;
     * @li ed2: parameterNumber, paramterCategory, discipline
     */
    const std::vector<long>& getParameterIds() const;
    const std::string& getTypeOfGrid() const;
    const GridDefinition& getGridDefinition() const;
    /**
     * Read the data from the underlying source to the vector data.
     * Data of at maximum data.size() will be read.
     * @param data the storage the data will be read to
     * @param missingValue the missing- / fill-value the returned data will have
     * @return the actual amount of data read
     */
    size_t readData(std::vector<double>& data, double missingValue) const;
    /**
     * Read the level-data from the underlying source to the vector levelData. In contrast to readData(), the
     * levelData does not need to be pre-allocated, since levelData usually are small (a few hundred (in grib1 limited to 256)).
     * @param levelData the storage the data will be read to
     * @param missingValue the missing- / fill-value the returned data will have
     * @return the actual amount of data read
     */
    size_t readLevelData(std::vector<double>& levelData, double missingValue) const;
private:
    std::string fileURL_;
    size_t filePos_;
    size_t msgPos_; // for multiMessages: multimessages
    std::string parameterName_;
    std::string shortName_;
    // ed1: indicatorOfParameter, gribTablesVersionNo, identificationOfOriginatingGeneratingCentre;
    // ed2: parameterNumber, paramterCategory, discipline
    std::vector<long> gridParameterIds_;
    long edition_;
    long dataTime_;
    long dataDate_;
    std::string stepUnits_;
    std::string stepType_;
    long stepStart_;
    long stepEnd_;
    long timeRangeIndicator_;
    long levelType_;
    long levelNo_;
    long perturbationNo_;
    long totalNumberOfEnsembles_;
    std::string typeOfGrid_;
    GridDefinition gridDefinition_;
};

/// Functor to find Messages with equal time
class GribFileMessageEqualTime : public std::unary_function<bool, const GribFileMessage&> {
public:
    GribFileMessageEqualTime(boost::posix_time::ptime time) : time_(time) {}
    ~GribFileMessageEqualTime() {}
    bool operator()(const GribFileMessage& gfm) { return gfm.getValidTime() == time_; }
private:
    boost::posix_time::ptime time_;
};

/// Functor to find messages with equal level and time
class GribFileMessageEqualLevelTime : public std::unary_function<bool, const GribFileMessage&> {
public:
    GribFileMessageEqualLevelTime(long edition, long levelType, long levelNo, boost::posix_time::ptime time) : edition_(edition), levelType_(levelType), levelNo_(levelNo), time_(time) {}
    ~GribFileMessageEqualLevelTime() {}
    bool operator()(const GribFileMessage& gfm) { return (gfm.getEdition() == edition_) && (gfm.getLevelType() == levelType_) && (gfm.getLevelNumber() == levelNo_) && (gfm.getValidTime() == time_); }
private:
    long edition_;
    long levelType_;
    long levelNo_;
    boost::posix_time::ptime time_;
};


class GribFileIndex
{
public:
    GribFileIndex();
    /**
     * Initialize the gribFileIndex for the gribFile gribFilePath.
     * If ignoreExistingXml = false, searches for existing indexes in
     * @li file.grbml
     * @li ENV{GRIB_FILE_INDEX}/file.grbml
     *
     * Otherwise, it parses the grib-file and creates a index in memory.
     *
     * Performance for getting an index of a 150MB grib-file with some 10s of messages:
     * @li remote NFS file, first time: 16s
     * @li file completely in memory: 1.1s
     * @li xml-file: 0.1s
     */
    GribFileIndex(boost::filesystem::path gribFilePath, const std::vector<std::string>& members, bool ignoreExistingXml = false);
    virtual ~GribFileIndex();
    const std::vector<GribFileMessage>& listMessages() const {return messages_;}
    const std::string& getUrl() const {return url_;}
private:
    std::string url_;
    std::vector<GribFileMessage> messages_;
    void initByGrib(boost::filesystem::path gribFilePath, const std::vector<std::string>& members);
    void initByXML(boost::filesystem::path xmlFilePath);
};

/// outputstream for a GribFileMessage
std::ostream& operator<<(std::ostream& os, const GribFileMessage& gfm);
/// outputstream for a GribFileIndex
std::ostream& operator<<(std::ostream& os, const GribFileIndex& gfm);


}


#endif /* GRIBFILEINDEX_H_ */
