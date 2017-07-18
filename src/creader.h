#ifndef CREADER_H
#define CREADER_H


#include <QObject>
#include <QString>

#include "ltkcpp.h"


class CTagInfo
{
public:
    CTagInfo(void);
    void clear(void);
    int antennaId;
    unsigned long long getTimeStampUSec(void) const;
    double getTimeStampSec(void) const;
    QList<unsigned char> data;

    unsigned long long timeStampUSec;
};

Q_DECLARE_METATYPE(CTagInfo);


class CReader : public QObject
{
    Q_OBJECT
public:
    explicit CReader(QString readerHostName, int verbose = 0);
    ~CReader(void);
    int processRecentChipsSeen(void);
signals:
    void newTag(const CTagInfo&);
    void newLogMessage(const QString&);

private:
    int _verbose;
    LLRP::CConnection *pConnectionToReader;
    LLRP::CTypeRegistry *pTypeRegistry;
    int checkConnectionStatus(void);
    LLRP::CMessage *recvMessage(int nMaxMS);
    void printXMLMessage(LLRP::CMessage *pMessage);
    int scrubConfiguration(void);
    int resetConfigurationToFactoryDefaults(void);
    int deleteAllROSpecs(void);
    int addROSpec(void);
    int enableROSpec(void);
    int startROSpec(void);
    void handleReaderEventNotification(LLRP::CReaderEventNotificationData *pNtfData);
    void handleAntennaEvent(LLRP::CAntennaEvent *pAntennaEvent);
    void handleReaderExceptionEvent(LLRP::CReaderExceptionEvent *pReaderExceptionEvent);
    int checkLLRPStatus(LLRP::CLLRPStatus *pLLRPStatus, const char *pWhatStr);
    LLRP::CMessage *transact (LLRP::CMessage *pSendMsg);
    int sendMessage(LLRP::CMessage *pSendMsg);
    int awaitReports(void);
    void processTagList(LLRP::CRO_ACCESS_REPORT *pRO_ACCESS_REPORT);
    void processTagInfo(LLRP::CTagReportData *pTagReportData);
};

#endif // CREADER_H
