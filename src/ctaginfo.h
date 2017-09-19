//********************************************************************
//    created:    2017-08-30 10:35 PM
//    file:       ctaginfo.h
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


#ifndef LLRPLAPS_CTAGINFO_H
#define LLRPLAPS_CTAGINFO_H

#include <cstdint>

#include <QObject>

namespace LLRPLaps
{
   class CTagInfo
    {
    public:
        CTagInfo();

        void clear();

        int AntennaId;

        u_int64_t getTimeStampUSec() const;

        double getTimeStampSec() const;

        QList<unsigned char> data;

    private:
        u_int64_t _timeStampUSec;
    };
}
#endif //LLRPLAPS_CTAGINFO_H
