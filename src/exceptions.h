//********************************************************************
//    created:    2017-08-30 10:40 PM
//    file:       exceptions.h
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


#ifndef LLRPLAPS_EXCEPTIONS_H
#define LLRPLAPS_EXCEPTIONS_H

#include <stdexcept>
#include <string>
#include <ltkcpp_base.h>

namespace LLRPLaps
{
    class ReaderException : public std::runtime_error
    {
    public:
        explicit ReaderException() noexcept;
        explicit ReaderException(const std::string& what) noexcept;
        explicit ReaderException(const char* what) noexcept;
    };

    class ReaderErrorDetailsException : public ReaderException
    {
    public:
        explicit ReaderErrorDetailsException(const std::string& what);
        explicit ReaderErrorDetailsException(const char* what);
        static std::string CErrorDetailsToString(const LLRP::CErrorDetails *errorDetails, const char* messageName, const char* function);
    };
}
#endif //LLRPLAPS_EXCEPTIONS_H
