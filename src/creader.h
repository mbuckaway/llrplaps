//********************************************************************
//      created:        2017/07/15
//      filename:       CREADER.CPP
//
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

#ifndef CREADER_H
#define CREADER_H


#include <cstdint>
#include <memory>

#include <QObject>
#include <QString>

#include "ltkcpp.h"
#include "ctaginfo.h"

class LLRPLaps::CTagInfo;

Q_DECLARE_METATYPE(LLRPLaps::CTagInfo);

namespace LLRPLaps
{

    class CReader : public QObject
    {
    Q_OBJECT
    public:
        explicit CReader(QString readerHostName);

        ~CReader() override;

        void ProcessRecentChipsSeen();

        void Connect();

    signals:

        void newTag(const LLRPLaps::CTagInfo &);

    private:
        std::shared_ptr<LLRP::CConnection> _connectionToReader;
        LLRP::CTypeRegistry* _typeRegistry;
        QString _readerHostname;

        void checkConnectionStatus();

        std::shared_ptr<LLRP::CMessage> recvMessage(int nMaxMS);

        void printXMLMessage(std::shared_ptr<LLRP::CMessage> message);

        void scrubConfiguration();

        void resetConfigurationToFactoryDefaults();

        void deleteAllROSpecs();

        void addROSpec();

        void enableROSpec();

        void startROSpec();

        void handleReaderEventNotification(LLRP::CReaderEventNotificationData *cReaderEventNotificationData);

        void handleAntennaEvent(LLRP::CAntennaEvent *antennaEvent);

        void handleReaderExceptionEvent(LLRP::CReaderExceptionEvent *readerExceptionEvent);

        void checkLLRPStatus(LLRP::CLLRPStatus* llrpStatus, const std::string whatStr);

        std::shared_ptr<LLRP::CMessage> transact(std::shared_ptr<LLRP::CMessage> sendMsg);

        void sendMessage(std::shared_ptr<LLRP::CMessage> sendMsg);

        void awaitReports();

        void processTagList(std::shared_ptr<LLRP::CRO_ACCESS_REPORT> RO_ACCESS_REPORT);

        void processTagInfo(LLRP::CTagReportData *tagReportData);

        std::string CErrorDetailsToString(const LLRP::CErrorDetails *errorDetails);

        const static int TIMEOUT_10SEC;
        const static int TIMEOUT_7SEC;
        const static int TIMEOUT_5SEC;
    };

}
#endif // CREADER_H
