//********************************************************************
//    created:    2017-08-30 10:46 PM
//    file:       exceptions.cpp
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

#include "exceptions.h"

#include <QString>

namespace LLRPLaps
{
    ReaderException::ReaderException() : std::runtime_error("Reader Error")
    {
    }

    ReaderException::ReaderException(const std::string& what) : std::runtime_error("Reader Error: " + what)
    {
    }

    ReaderException::ReaderException(const char* what) : std::runtime_error("Reader Error: " + std::string(what))
    {
    }

    ReaderErrorDetailsException::ReaderErrorDetailsException(const char* what) : ReaderException(what)
    {

    }

    ReaderErrorDetailsException::ReaderErrorDetailsException(const std::string& what) : ReaderException(what)
    {

    }

    std::string ReaderErrorDetailsException::CErrorDetailsToString(
            const LLRP::CErrorDetails *errorDetails, const char *messageName, const char *function)
    {
        std::string what;
        what.append("ERROR: ");
        what.append(messageName);
        what.append(" ");
        what.append(function);
        what.append(" failed: ");
        what.append(errorDetails->m_pWhatStr);

        if (NULL != errorDetails->m_pRefType)
        {
            what.append(": Reference type ");
            what.append(errorDetails->m_pRefType->m_pName);
        }

        if (NULL != errorDetails->m_pRefField)
        {
            what.append(": Reference field ");
            what.append(errorDetails->m_pRefField->m_pName);
        }
    }
}
