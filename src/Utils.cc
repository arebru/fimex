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

#include "fimex/Utils.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <exception>
#include <cmath>
#include <iomanip>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace MetNoFimex
{

int round(double num) {
    return static_cast<int>(num < 0.0 ? std::ceil(num - 0.5) : std::floor(num + 0.5));
}

std::string trim(const std::string& str) {
	int pos1 = str.find_first_not_of(" ");
	int pos2 = str.find_last_not_of(" ");
	return str.substr(pos1, pos2+1);
}

std::string string2lowerCase(const std::string& str)
{
	std::string s(str);
	for (unsigned int i = 0; i < s.length(); i++) {
		s[i] = std::tolower(s[i]);
	}
	return s;
}

/**
 * specialization for high prececision
 */
template<>
std::string type2string<double>(double in)
{
    std::ostringstream buffer;
    buffer << std::setprecision(24) << in;
    return buffer.str();
}

/**
 * speicalizations for functionality of real isnan
 * @param x
 * @return
 */
template<>
int mifi_isnan<double>(double x) {
    return mifi_isnand(x);
}

template<>
int mifi_isnan<float>(float x) {
    return false;//mifi_isnanf(x);
}


std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters)
{
	std::vector<std::string> tokens;
	// skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // find first "non-delimiter".
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos) {
        // found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
    return tokens;
}

static boost::posix_time::ptime epochBase(boost::gregorian::date(1970, boost::date_time::Jan, 1));
epoch_seconds posixTime2epochTime(const boost::posix_time::ptime& time)
{
    return (time - epochBase).total_seconds();
}

}
