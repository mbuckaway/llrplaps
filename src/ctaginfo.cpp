//********************************************************************
//    created:    2017-08-30 10:35 PM
//    file:       ctaginfo.cpp
//  (C) Copyright 2017 Forestcity Velodrome
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*********************************************************************

#include "ctaginfo.h"

namespace LLRPLaps
{
    CTagInfo::CTagInfo(void) : _timeStampUSec(0LL), AntennaId(0)
    {
        data.clear();
    }


    void CTagInfo::clear(void)
    {
        data.clear();
        _timeStampUSec = 0;
        AntennaId = 0;
    }


    u_int64_t CTagInfo::getTimeStampUSec(void) const
    {
        return _timeStampUSec;
    }


    double CTagInfo::getTimeStampSec(void) const
    {
        return static_cast<double>(_timeStampUSec / 1000000.0);
    }

}