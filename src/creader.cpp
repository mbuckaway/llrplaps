// creader.cpp


#include <memory>
#include <QList>

#include <ltkcpp_platform.h>
#include <ltkcpp.h>
#include "exceptions.h"
#include "creader.h"
#include "ctaginfo.h"


namespace LLRPLaps
{
    const int CReader::TIMEOUT_10SEC = 10000;
    const int CReader::TIMEOUT_7SEC = 7000;
    const int CReader::TIMEOUT_5SEC = 5000;

    CReader::CReader(QString readerHostName): _readerHostname (readerHostName), _connectionToReader(nullptr), _typeRegistry(nullptr)
    {
    }

    void CReader::Connect()
    {
        /*
         * Allocate the type registry. This is needed
         * by the connection to decode.
         */

        _typeRegistry = LLRP::getTheTypeRegistry();
        if (!_typeRegistry)
        {
            throw LLRPLaps::ReaderException("ERROR: getTheTypeRegistry failed");
        }

        /*
         * Construct a connection (LLRP::CConnection).
         * Using a 32kb max frame size for send/recv.
         * The connection object is ready for business
         * but not actually connected to the reader yet.
         */

        _connectionToReader = std::make_shared<LLRP::CConnection>(new LLRP::CConnection(_typeRegistry, 32u * 1024u));
        if (!_connectionToReader.get())
        {
            throw LLRPLaps::ReaderException("ERROR: new CConnection failed");
        }

        /*
         * Open the connection to the reader
         */

        if (_connectionToReader->openConnectionToReader(_readerHostname.toLatin1().data()))
        {
            throw LLRPLaps::ReaderException(QString().sprintf("ERROR: connect: %s", _connectionToReader->getConnectError()).toStdString());
        }

        /*
         * Commence the sequence and check for errors as we go.
         * See comments for each routine for details.
         * Each routine prints messages.
         */

        checkConnectionStatus();
        scrubConfiguration();
        addROSpec();
        enableROSpec();
    }

    void CReader::ProcessRecentChipsSeen()
    {
        startROSpec();
        awaitReports();
    }


    CReader::~CReader(void)
    {
        // Don't blow exceptions in the destructor
        try
        {
            scrubConfiguration();
            _connectionToReader->closeConnectionToReader();
        }
        catch (const std::exception& e)
        {
            // Log exception when logging is implemented
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Await and check the connection status message from the reader
 **
 ** We are expecting a READER_EVENT_NOTIFICATION message that
 ** tells us the connection is OK. The reader is suppose to
 ** send the message promptly upon connection.
 **
 ** If there is already another LLRP connection to the
 ** reader we'll get a bad Status.
 **
 ** The message should be something like:
 **
 **     <READER_EVENT_NOTIFICATION MessageID='0'>
 **       <ReaderEventNotificationData>
 **         <UTCTimestamp>
 **           <Microseconds>1184491439614224</Microseconds>
 **         </UTCTimestamp>
 **         <ConnectionAttemptEvent>
 **           <Status>Success</Status>
 **         </ConnectionAttemptEvent>
 **       </ReaderEventNotificationData>
 **     </READER_EVENT_NOTIFICATION>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

    void CReader::checkConnectionStatus()
    {
        LLRP::CREADER_EVENT_NOTIFICATION *creaderEventNotification;

        /*
         * Expect the notification within 10 seconds.
         * It is suppose to be the very first message sent.
         */
        std::shared_ptr<LLRP::CMessage> message = recvMessage(TIMEOUT_10SEC);

        /*
         * recvMessage() returns NULL if something went wrong.
         */

        if (nullptr == message.get())
        {
            /* recvMessage already tattled */
            throw LLRPLaps::ReaderConnectionException("recvMessage failed: No connection");
        }

        /*
         * Check to make sure the message is of the right type.
         * The type label (pointer) in the message should be
         * the type descriptor for READER_EVENT_NOTIFICATION.
         */

        if (&LLRP::CREADER_EVENT_NOTIFICATION::s_typeDescriptor != message->m_pType)
        {
            throw LLRPLaps::ReaderConnectionException("recvMessage failed: Wrong message type");
        }

        /*
         * Now that we are sure it is a READER_EVENT_NOTIFICATION,
         * traverse to the ReaderEventNotificationData parameter.
         */

        creaderEventNotification = dynamic_cast<LLRP::CREADER_EVENT_NOTIFICATION*>(message.get());
        auto *readerEventNotificationData = creaderEventNotification->getReaderEventNotificationData();

        if (nullptr == readerEventNotificationData)
        {
            throw LLRPLaps::ReaderConnectionException("recvMessage failed: Wrong message type");
        }

        /*
         * The ConnectionAttemptEvent parameter must be present.
         */

        auto *connectionAttemptEvent = readerEventNotificationData->getConnectionAttemptEvent();
        if (NULL == connectionAttemptEvent)
        {
            throw LLRPLaps::ReaderConnectionException("recvMessage failed: Connection parameter not present");
        }

        /*
         * The status in the ConnectionAttemptEvent parameter
         * must indicate connection success.
         */

        if (LLRP::ConnectionAttemptStatusType_Success != connectionAttemptEvent->getStatus())
        {
            throw LLRPLaps::ReaderConnectionException("recvMessage failed: invalid connection");
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Scrub the reader configuration
 **
 ** The steps:
 **     - Try to reset configuration to factory defaults,
 **       this feature is optional and may not be supported
 **       by the reader.
 **     - Delete all ROSpecs
 **
 *****************************************************************************/

    void CReader::scrubConfiguration()
    {
        resetConfigurationToFactoryDefaults();
        deleteAllROSpecs();
    }


/**
 *****************************************************************************
 **
 ** @brief  Send a SET_READER_CONFIG message that resets the
 **         reader to factory defaults.
 **
 ** NB: The ResetToFactoryDefault semantics vary between readers.
 **     It might have no effect because it is optional.
 **
 ** The message is:
 **
 **     <SET_READER_CONFIG MessageID='101'>
 **       <ResetToFactoryDefault>1</ResetToFactoryDefault>
 **     </SET_READER_CONFIG>
 **
 *****************************************************************************/

    void CReader::resetConfigurationToFactoryDefaults()
    {
        /*
         * Compose the command message
         */

        std::shared_ptr<LLRP::CSET_READER_CONFIG> readerConfig (new LLRP::CSET_READER_CONFIG());
        readerConfig->setMessageID(101);
        readerConfig->setResetToFactoryDefault(1);

        /*
         * Send the message, expect the response of certain type
         */

        std::shared_ptr<LLRP::CMessage> responseMessage = transact(readerConfig);

        /*
         * transact() returns NULL if something went wrong.
         */

        if (nullptr == responseMessage.get())
        {
            /* transact already tattled */
            throw LLRPLaps::ReaderException("transact command failed");
        }

        /*
         * Cast to a SET_READER_CONFIG_RESPONSE message.
         */

        auto *configResponse = dynamic_cast<LLRP::CSET_READER_CONFIG_RESPONSE *>(responseMessage.get());

        /*
         * Check the LLRPStatus parameter.
         */

        checkLLRPStatus(configResponse->getLLRPStatus(), "resetConfigurationToFactoryDefaults");
    }


/**
 *****************************************************************************
 **
 ** @brief  Delete all ROSpecs using DELETE_ROSPEC message
 **
 ** Per the spec, the DELETE_ROSPEC message contains an ROSpecID
 ** of 0 to indicate we want all ROSpecs deleted.
 **
 ** The message is
 **
 **     <DELETE_ROSPEC MessageID='102'>
 **       <ROSpecID>0</ROSpecID>
 **     </DELETE_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

    void CReader::deleteAllROSpecs()
    {
        /*
         * Compose the command message
         */

        std::shared_ptr<LLRP::CDELETE_ROSPEC> cdeleteRospec (new LLRP::CDELETE_ROSPEC());
        cdeleteRospec->setMessageID(102);
        cdeleteRospec->setROSpecID(0);               /* All */

        /*
         * Send the message, expect the response of certain type
         */

        std::shared_ptr<LLRP::CMessage> responseMessage = transact(cdeleteRospec);

       /*
         * transact() returns NULL if something went wrong.
         */

        if (nullptr == responseMessage)
        {
            /* transact already tattled */
            throw LLRPLaps::ReaderException("transact could not create DELETE_ROSPEC command");
        }

        /*
         * Cast to a DELETE_ROSPEC_RESPONSE message.
         */

        auto *cdeleteRospecResponse = dynamic_cast<LLRP::CDELETE_ROSPEC_RESPONSE *>(responseMessage.get());

        /*
         * Check the LLRPStatus parameter.
         */

        checkLLRPStatus(cdeleteRospecResponse->getLLRPStatus(), "deleteAllROSpecs");
    }


/**
 *****************************************************************************
 **
 ** @brief  Add our ROSpec using ADD_ROSPEC message
 **
 ** This ROSpec waits for a START_ROSPEC message,
 ** then takes inventory on all antennas for 5 seconds.
 **
 ** The tag report is generated after the ROSpec is done.
 **
 ** This example is deliberately streamlined.
 ** Nothing here configures the antennas, RF, or Gen2.
 ** The current defaults are used. Remember we just reset
 ** the reader to factory defaults (above). Normally an
 ** application would be more precise in configuring the
 ** reader and in its ROSpecs.
 **
 ** Experience suggests that typical ROSpecs are about
 ** double this in size.
 **
 ** The message is
 **
 **     <ADD_ROSPEC MessageID='201'>
 **       <ROSpec>
 **         <ROSpecID>123</ROSpecID>
 **         <Priority>0</Priority>
 **         <CurrentState>Disabled</CurrentState>
 **         <ROBoundarySpec>
 **           <ROSpecStartTrigger>
 **             <ROSpecStartTriggerType>Null</ROSpecStartTriggerType>
 **           </ROSpecStartTrigger>
 **           <ROSpecStopTrigger>
 **             <ROSpecStopTriggerType>Null</ROSpecStopTriggerType>
 **             <DurationTriggerValue>0</DurationTriggerValue>
 **           </ROSpecStopTrigger>
 **         </ROBoundarySpec>
 **         <AISpec>
 **           <AntennaIDs>0</AntennaIDs>
 **           <AISpecStopTrigger>
 **             <AISpecStopTriggerType>Duration</AISpecStopTriggerType>
 **             <DurationTrigger>5000</DurationTrigger>
 **           </AISpecStopTrigger>
 **           <InventoryParameterSpec>
 **             <InventoryParameterSpecID>1234</InventoryParameterSpecID>
 **             <ProtocolID>EPCGlobalClass1Gen2</ProtocolID>
 **           </InventoryParameterSpec>
 **         </AISpec>
 **         <ROReportSpec>
 **           <ROReportTrigger>Upon_N_Tags_Or_End_Of_ROSpec</ROReportTrigger>
 **           <N>0</N>
 **           <TagReportContentSelector>
 **             <EnableROSpecID>0</EnableROSpecID>
 **             <EnableSpecIndex>0</EnableSpecIndex>
 **             <EnableInventoryParameterSpecID>0</EnableInventoryParameterSpecID>
 **             <EnableAntennaID>0</EnableAntennaID>
 **             <EnableChannelIndex>0</EnableChannelIndex>
 **             <EnablePeakRSSI>0</EnablePeakRSSI>
 **             <EnableFirstSeenTimestamp>0</EnableFirstSeenTimestamp>
 **             <EnableLastSeenTimestamp>0</EnableLastSeenTimestamp>
 **             <EnableTagSeenCount>0</EnableTagSeenCount>
 **             <EnableAccessSpecID>0</EnableAccessSpecID>
 **           </TagReportContentSelector>
 **         </ROReportSpec>
 **       </ROSpec>
 **     </ADD_ROSPEC>
 **
 ** @throws    ReaderException on error
 **
 *****************************************************************************/

    void CReader::addROSpec(void)
    {
        QString s;

        LLRP::CROSpecStartTrigger *pROSpecStartTrigger = new LLRP::CROSpecStartTrigger();
        pROSpecStartTrigger->setROSpecStartTriggerType(
                LLRP::ROSpecStartTriggerType_Null);

        LLRP::CROSpecStopTrigger *pROSpecStopTrigger = new LLRP::CROSpecStopTrigger();
        pROSpecStopTrigger->setROSpecStopTriggerType(LLRP::ROSpecStopTriggerType_Null);
        pROSpecStopTrigger->setDurationTriggerValue(0);     /* n/a */

        LLRP::CROBoundarySpec *pROBoundarySpec = new LLRP::CROBoundarySpec();
        pROBoundarySpec->setROSpecStartTrigger(pROSpecStartTrigger);
        pROBoundarySpec->setROSpecStopTrigger(pROSpecStopTrigger);

        LLRP::CAISpecStopTrigger *pAISpecStopTrigger = new LLRP::CAISpecStopTrigger();
        pAISpecStopTrigger->setAISpecStopTriggerType(
                LLRP::AISpecStopTriggerType_Duration);
        pAISpecStopTrigger->setDurationTrigger(500);

        LLRP::CInventoryParameterSpec *pInventoryParameterSpec =
                new LLRP::CInventoryParameterSpec();
        pInventoryParameterSpec->setInventoryParameterSpecID(1234);
        pInventoryParameterSpec->setProtocolID(LLRP::AirProtocols_EPCGlobalClass1Gen2);

        // FIXME: Cannot assume all antennas
        LLRP::llrp_u16v_t antennaIDs = LLRP::llrp_u16v_t(1);
        antennaIDs.m_pValue[0] = 0;         /* All */

        LLRP::CAISpec *pAISpec = new LLRP::CAISpec();
        pAISpec->setAntennaIDs(antennaIDs);
        pAISpec->setAISpecStopTrigger(pAISpecStopTrigger);
        pAISpec->addInventoryParameterSpec(pInventoryParameterSpec);

        LLRP::CTagReportContentSelector *pTagReportContentSelector =
                new LLRP::CTagReportContentSelector();
        pTagReportContentSelector->setEnableROSpecID(FALSE);
        pTagReportContentSelector->setEnableSpecIndex(FALSE);
        pTagReportContentSelector->setEnableInventoryParameterSpecID(FALSE);
        pTagReportContentSelector->setEnableAntennaID(TRUE);
        pTagReportContentSelector->setEnableChannelIndex(FALSE);
        pTagReportContentSelector->setEnablePeakRSSI(FALSE);
        pTagReportContentSelector->setEnableFirstSeenTimestamp(TRUE);
        pTagReportContentSelector->setEnableLastSeenTimestamp(FALSE);
        pTagReportContentSelector->setEnableTagSeenCount(FALSE);
        pTagReportContentSelector->setEnableAccessSpecID(FALSE);

        LLRP::CROReportSpec *pROReportSpec = new LLRP::CROReportSpec();
        pROReportSpec->setROReportTrigger(
                LLRP::ROReportTriggerType_Upon_N_Tags_Or_End_Of_ROSpec);
        pROReportSpec->setN(0);         /* Unlimited */
        pROReportSpec->setTagReportContentSelector(pTagReportContentSelector);

        LLRP::CROSpec *pROSpec = new LLRP::CROSpec();
        pROSpec->setROSpecID(123);
        pROSpec->setPriority(0);
        pROSpec->setCurrentState(LLRP::ROSpecState_Disabled);
        pROSpec->setROBoundarySpec(pROBoundarySpec);
        pROSpec->addSpecParameter(pAISpec);
        pROSpec->setROReportSpec(pROReportSpec);

        ;

        /*
         * Compose the command message.
         * N.B.: After the message is composed, all the parameters
         *       constructed, immediately above, are considered "owned"
         *       by the command message. When it is destructed so
         *       too will the parameters be.
         */

        std::shared_ptr<LLRP::CADD_ROSPEC> command (new LLRP::CADD_ROSPEC());
        command->setMessageID(201);
        command->setROSpec(pROSpec);

        /*
         * Send the message, expect the response of certain type
         */

        std::shared_ptr<LLRP::CMessage> responseMessage = transact(command);

        /*
         * transact() returns NULL if something went wrong.
         */

        if (nullptr == responseMessage.get())
        {
            /* transact already tattled */
            throw LLRPLaps::ReaderException("transact failed on addROSpec");
        }

        /*
         * Cast to a ADD_ROSPEC_RESPONSE message.
         */

        LLRP::CADD_ROSPEC_RESPONSE *pROSpecResponseMessage = dynamic_cast<LLRP::CADD_ROSPEC_RESPONSE*>(responseMessage.get());

        /*
         * Check the LLRPStatus parameter.
         */

        checkLLRPStatus(pROSpecResponseMessage->getLLRPStatus(), "addROSpec");
    }


/**
 *****************************************************************************
 **
 ** @brief  Enable our ROSpec using ENABLE_ROSPEC message
 **
 ** Enable the ROSpec that was added above.
 **
 ** The message we send is:
 **     <ENABLE_ROSPEC MessageID='202'>
 **       <ROSpecID>123</ROSpecID>
 **     </ENABLE_ROSPEC>
 **
 ** @throws     ReaderException on error
 **
 *****************************************************************************/

    void CReader::enableROSpec()
    {
        /*
         * Compose the command message
         */

        std::shared_ptr<LLRP::CENABLE_ROSPEC> command (new LLRP::CENABLE_ROSPEC());
        command->setMessageID(202);
        command->setROSpecID(123);

        /*
         * Send the message, expect the response of certain type
         */

        std::shared_ptr<LLRP::CMessage> responseMessage = transact(command);

        /*
         * transact() returns NULL if something went wrong.
         */

        if (nullptr == responseMessage.get())
        {
            /* transact already tattled */
            throw LLRPLaps::ReaderException("Response to ENABLE_ROSPEC command failed");
        }

        /*
         * Cast to a ENABLE_ROSPEC_RESPONSE message.
         */

        auto *rospecResponse = dynamic_cast<LLRP::CENABLE_ROSPEC_RESPONSE *>(responseMessage.get());

        /*
         * Check the LLRPStatus parameter.
         */

        checkLLRPStatus(rospecResponse->getLLRPStatus(), "enableROSpec");
    }


/**
 *****************************************************************************
 **
 ** @brief  Start our ROSpec using START_ROSPEC message
 **
 ** Start the ROSpec that was added above.
 **
 ** The message we send is:
 **     <START_ROSPEC MessageID='203'>
 **       <ROSpecID>123</ROSpecID>
 **     </START_ROSPEC>
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

    void CReader::startROSpec(void)
    {
        /*
         * Compose the command message
         */

        std::shared_ptr<LLRP::CSTART_ROSPEC> cstartRospecCommand (new LLRP::CSTART_ROSPEC());
        cstartRospecCommand->setMessageID(202);
        cstartRospecCommand->setROSpecID(123);

        /*
         * Send the message, expect the response of certain type
         */

        std::shared_ptr<LLRP::CMessage> responseMessage = transact(cstartRospecCommand);

       /*
         * transact() returns NULL if something went wrong.
         */

        if (nullptr == responseMessage.get())
        {
            /* transact already tattled */
            throw LLRPLaps::ReaderException("Response to START_ROSPEC command invalid");
        }

        /*
         * Cast to a START_ROSPEC_RESPONSE message.
         */

        auto *response = dynamic_cast<LLRP::CSTART_ROSPEC_RESPONSE *>(responseMessage.get());

        /*
         * Check the LLRPStatus parameter.
         */

        checkLLRPStatus(response->getLLRPStatus(), "startROSpec");
    }

/**
 *****************************************************************************
 **
 ** @brief  Receive the RO_ACCESS_REPORT
 **
 ** Receive messages until an RO_ACCESS_REPORT is received.
 ** Time limit is 7 seconds. We expect a report within 5 seconds.
 **
 ** This shows how to determine the type of a received message.
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong
 **
 *****************************************************************************/

    void CReader::awaitReports()
    {
        bool done(false);

        /*
         * Keep receiving messages until done or until
         * something bad happens.
         */

        while (!done)
        {
            const LLRP::CTypeDescriptor *pType;

            /*
             * Wait up to 7 seconds for a message. The report
             * should occur within 5 seconds.
             */

            auto recvMessage = recvMessage(TIMEOUT_7SEC);
            if (nullptr == recvMessage.get())
            {
                /*
                 * Did not receive a message within a reasonable
                 * amount of time. recvMessage() already tattled
                 */
                throw LLRPLaps::ReaderTimeoutException("timeout waiting for recvMessage awating reports");
            }

            /*
             * What happens depends on what kind of message
             * received. Use the type label (m_pType) to
             * discriminate message types.
             */

            pType = recvMessage->m_pType;

            /*
             * Is it a tag report? If so, process.
             */

            if (&LLRP::CRO_ACCESS_REPORT::s_typeDescriptor == pType)
            {
                processTagList(dynamic_cast<LLRP::CRO_ACCESS_REPORT *>(recvMessage));
                done = true;
            }

                /*
                 * Is it a reader event? This example only recognizes
                 * AntennaEvents.
                 */

            else if (&LLRP::CREADER_EVENT_NOTIFICATION::s_typeDescriptor == pType)
            {
                auto *creaderEventNotification = (LLRP::CREADER_EVENT_NOTIFICATION *) recvMessage;
                auto *readerEventNotificationData = creaderEventNotification->getReaderEventNotificationData();
                if (nullptr != readerEventNotificationData)
                {
                    handleReaderEventNotification(readerEventNotificationData);
                }
                else
                {
                    /*
                     * This should never happen. Using continue
                     * to keep indent depth down.
                     */

                    // LOG(QString().sprintf("WARNING: READER_EVENT_NOTIFICATION without data"));
                }
            }

                /*
                 * Hmmm. Something unexpected. Just tattle and keep going.
                 */

            else
            {
                // LOG(QString().sprintf("WARNING: Ignored unexpected message during monitor: %s", pType->m_pName));
            }

        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to
 **
 ** The report is printed in list order, which is arbitrary.
 **
 ** TODO: It would be cool to sort the list by EPC and antenna,
 **       then print it.
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::processTagList(std::shared_ptr<LLRP::CRO_ACCESS_REPORT> RO_ACCESS_REPORT)
    {
        for (std::list<LLRP::CTagReportData*>::iterator i = RO_ACCESS_REPORT->beginTagReportData(); RO_ACCESS_REPORT->endTagReportData() != i; ++i)
        {
            processTagInfo(*i);
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to print one tag report entry on one line
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::processTagInfo(LLRP::CTagReportData *tagReportData)
    {
        const LLRP::CTypeDescriptor *cTypeDescriptor;
        auto *pEPCParameter = tagReportData->getEPCParameter();
        LLRPLaps::CTagInfo tagInfo;

        /*
         * Process the EPC. It could be a 96-bit EPC_96 parameter
         * or an variable length EPCData parameter.
         */

        if (nullptr != pEPCParameter)
        {
            LLRP::llrp_u96_t my_u96;
            LLRP::llrp_u1v_t my_u1v;
            LLRP::llrp_u8_t *value = NULL;
            int n;

            cTypeDescriptor = pEPCParameter->m_pType;
            if (&LLRP::CEPC_96::s_typeDescriptor == cTypeDescriptor)
            {
                auto *pEPC_96 = dynamic_cast<LLRP::CEPC_96 *>(pEPCParameter);
                my_u96 = pEPC_96->getEPC();
                value = my_u96.m_aValue;
                n = 12u;
            }
            else if (&LLRP::CEPCData::s_typeDescriptor == cTypeDescriptor)
            {
                auto *pEPCData = dynamic_cast<LLRP::CEPCData *>(pEPCParameter);
                my_u1v = pEPCData->getEPC();
                value = my_u1v.m_pValue;
                n = (my_u1v.m_nBit + 7u) / 8u;
            }

            if (value)
            {
                tagInfo.setTimeStampUSec(tagReportData->getFirstSeenTimestampUTC()->getMicroseconds());
                tagInfo.AntennaId = tagReportData->getAntennaID()->getAntennaID();
                tagInfo.data.reserve(n);
                for (int i = 0; i < n; i++)
                {
                    tagInfo.data.push_back(value[i]);
                }
                emit newTag(tagInfo);
            }
            else
            {
                // LOG(QString("Unknown-epc-data-type in tag"));
            }
        }
        else
        {
            // LOGQString("Missing-epc-data in tag"));
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Handle a ReaderEventNotification
 **
 ** Handle the payload of a READER_EVENT_NOTIFICATION message.
 ** This routine simply dispatches to handlers of specific
 ** event types.
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::handleReaderEventNotification(LLRP::CReaderEventNotificationData *cReaderEventNotificationData)
    {
        auto reported = false;

        auto *pAntennaEvent = cReaderEventNotificationData->getAntennaEvent();
        if (NULL != pAntennaEvent)
        {
            handleAntennaEvent(pAntennaEvent);
            reported = true;
        }

        auto *pReaderExceptionEvent = cReaderEventNotificationData->getReaderExceptionEvent();
        if (NULL != pReaderExceptionEvent)
        {
            handleReaderExceptionEvent(pReaderExceptionEvent);
            reported = true;
        }

        /*
         * Similarly handle other events here:
         *      HoppingEvent
         *      GPIEvent
         *      ROSpecEvent
         *      ReportBufferLevelWarningEvent
         *      ReportBufferOverflowErrorEvent
         *      RFSurveyEvent
         *      AISpecEvent
         *      ConnectionAttemptEvent
         *      ConnectionCloseEvent
         *      Custom
         */

        if (0 == reported)
        {
            // LOG("NOTICE: Unexpected (unhandled) ReaderEvent"));
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Handle an AntennaEvent
 **
 ** An antenna was disconnected or (re)connected. Tattle.
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::handleAntennaEvent(LLRP::CAntennaEvent *pAntennaEvent)
    {
        LLRP::EAntennaEventType eEventType;
        LLRP::llrp_u16_t AntennaID;
        std::string stateStr;
        QString s;

        eEventType = pAntennaEvent->getEventType();
        AntennaID = pAntennaEvent->getAntennaID();

        switch (eEventType)
        {
            case LLRP::AntennaEventType_Antenna_Disconnected:
                stateStr = "disconnected";
                break;

            case LLRP::AntennaEventType_Antenna_Connected:
                stateStr = "connected";
                break;

            default:
                stateStr = "?unknown-event?";
                break;
        }

        // LOG(s.sprintf("NOTICE: Antenna %d is %s", AntennaID,ß stateStr.c_str()));
    }


/**
 *****************************************************************************
 **
 ** @brief  Handle a ReaderExceptionEvent
 **
 ** Something has gone wrong. There are lots of details but
 ** all this does is print the message, if one.
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::handleReaderExceptionEvent(LLRP::CReaderExceptionEvent *pReaderExceptionEvent)
    {
        LLRP::llrp_utf8v_t Message;
        QString s;

        Message = pReaderExceptionEvent->getMessage();

        if (0 < Message.m_nValue && NULL != Message.m_pValue)
        {
            // LOG(s.sprintf("NOTICE: ReaderException '%.*s'", Message.m_nValue, Message.m_pValue));
        }
        else
        {
            // LOG(emit newLogMessage(s.sprintf("NOTICE: ReaderException but no message"));
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Helper routine to check an LLRPStatus parameter
 **         and tattle on errors
 **
 ** Helper routine to interpret the LLRPStatus subparameter
 ** that is in all responses. It tattles on an error, if one,
 ** and tries to safely provide details.
 **
 ** This simplifies the code, above, for common check/tattle
 ** sequences.
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong, already tattled
 **
 *****************************************************************************/

    void CReader::checkLLRPStatus(LLRP::CLLRPStatus* llrpStatus, const std::string whatStr)
    {
        /*
         * The LLRPStatus parameter is mandatory in all responses.
         * If it is missing there should have been a decode error.
         * This just makes sure (remember, this program is a
         * diagnostic and suppose to catch LTKC mistakes).
         */

        if (nullptr == llrpStatus)
        {
            // LOG(s.sprintf("ERROR: %s missing LLRP status", pWhatStr));
            throw LLRPLaps::ReaderException(QString().sprintf("ERROR: %s missing LLRP status", whatStr).toStdString());
        }

        /*
         * Make sure the status is M_Success.
         * If it isn't, print the error string if one.
         * This does not try to pretty-print the status
         * code. To get that, run this program with -vv
         * and examine the XML output.
         */

        if (LLRP::StatusCode_M_Success != llrpStatus->getStatusCode())
        {
            LLRP::llrp_utf8v_t ErrorDesc;

            ErrorDesc = llrpStatus->getErrorDescription();
            QString errorStr;
            if (0 == ErrorDesc.m_nValue)
            {
                errorStr.sprintf("ERROR: %s failed, no error description given", whatStr));
            }
            else
            {
                errorStr.sprintf("ERROR: %s failed, %.*s", whatStr, ErrorDesc.m_nValue, ErrorDesc.m_pValue));
            }
            throw LLRPLaps::ReaderException(errorStr.toStdString());
        }
    }


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to do an LLRP transaction
 **
 ** Wrapper to transact a request/resposne.
 **     - Print the outbound message in XML if _verbose level is at least 2
 **     - Send it using the LLRP_Conn_transact()
 **     - LLRP_Conn_transact() receives the response or recognizes an error
 **     - Tattle on errors, if any
 **     - Print the received message in XML if _verbose level is at least 2
 **     - If the response is ERROR_MESSAGE, the request was sufficiently
 **       misunderstood that the reader could not send a proper reply.
 **       Deem this an error, free the message.
 **
 ** The message returned resides in allocated memory. It is the
 ** caller's obligtation to free it.
 **
 ** @return     ==NULL          Something went wrong, already tattled
 **             !=NULL          Pointer to a message
 **
 *****************************************************************************/

    std::shared_ptr<LLRP::CMessage> CReader::transact(std::shared_ptr<LLRP::CMessage> sendMsg)
    {
        std::shared_ptr<LLRP::CMessage> message;
        QString s;

        /*
         * Print the XML text for the outbound message if
         * verbosity is 2 or higher.
         */

        printXMLMessage(sendMsg);

        /*
         * Send the message, expect the response of certain type.
         * If LLRP::CConnection::transact() returns NULL then there was
         * an error. In that case we try to print the error details.
         */

        message = _connectionToReader->transact(sendMsg.get(), TIMEOUT_5SEC);

        if (nullptr == message)
        {
            const LLRP::CErrorDetails *pError = _connectionToReader->getTransactError();

            // LOG(s.sprintf("ERROR: %s transact failed, %s", sendMsg->m_pType->m_pName, pError->m_pWhatStr
            //                                                                                          ? pError->m_pWhatStr
            //                                                                                          : "no reason given"));

            if (nullptr != pError->m_pRefType)
            {
                // LOG(s.sprintf("ERROR: ... reference type %s", pError->m_pRefType->m_pName));
            }

            if (nullptr != pError->m_pRefField)
            {
                // LOG((s.sprintf("ERROR: ... reference field %s", pError->m_pRefField->m_pName));
            }

            return nullptr;
        }

        /*
         * Print the XML text for the inbound message if
         * verbosity is 2 or higher.
         */

        printXMLMessage(message);

        /*
         * If it is an ERROR_MESSAGE (response from reader
         * when it can't understand the request), tattle
         * and declare defeat.
         */

        if (&LLRP::CERROR_MESSAGE::s_typeDescriptor == message->m_pType)
        {
            const LLRP::CTypeDescriptor *pResponseType = sendMsg->m_pType->m_pResponseType;

            // LOG(s.sprintf("ERROR: Received ERROR_MESSAGE instead of %s", pResponseType->m_pName));
            message = nullptr;
        }

        return message;
    }


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to receive a message
 **
 ** This can receive notifications as well as responses.
 **     - Recv a message using the LLRP_Conn_recvMessage()
 **     - Tattle on errors, if any
 **     - Print the message in XML if _verbose level is at least 2
 **
 ** The message returned resides in allocated memory. It is the
 ** caller's obligtation to free it.
 **
 ** @param[in]  nMaxMS          -1 => block indefinitely
 **                              0 => just peek at input queue and
 **                                   socket queue, return immediately
 **                                   no matter what
 **                             >0 => ms to await complete frame
 **
 ** @return     !=NULL          Pointer to a message
 ** @throws     ReaderException on error
 **
 **
 *****************************************************************************/

    std::shared_ptr<LLRP::CMessage> CReader::recvMessage(int nMaxMS)
    {
        std::shared_ptr<LLRP::CMessage> message;
        QString s;

        /*
         * Receive the message subject to a time limit
         */

        message = _connectionToReader->recvMessage(nMaxMS);

        /*
         * If LLRP::CConnection::recvMessage() returns NULL then there was
         * an error. In that case we try to print the error details.
         */

        if (NULL == message.get())
        {
            const LLRP::CErrorDetails *pError = _connectionToReader->getRecvError();

            throw LLRPLaps::ReaderException(s.sprintf("ERROR: recvMessage failed, %s", pError->m_pWhatStr ? pError->m_pWhatStr
                                                                                             : "no reason given").toStdString());
        }

        // FIXME: REquired when logging is implemented:    printXMLMessage(message);

        return message;
    }


/**
 *****************************************************************************
 **
 ** @brief  Wrapper routine to send a message
 **
 ** Wrapper to send a message.
 **     - Print the message in XML if _verbose level is at least 2
 **     - Send it using the LLRP_Conn_sendMessage()
 **     - Tattle on errors, if any
 **
 ** @param[in]  sendMsg        Pointer to message to send
 **
 ** @return     ==0             Everything OK
 **             !=0             Something went wrong, already tattled
 **
 *****************************************************************************/

    void CReader::sendMessage(std::shared_ptr<LLRP::CMessage> sendMsg)
    {
        /*
         * Print the XML text for the outbound message if
         * verbosity is 2 or higher.
         */

        // FIXME: Enable for logging: printXMLMessage(sendMsg);

        /*
         * If LLRP::CConnection::sendMessage() returns other than RC_OK
         * then there was an error. In that case we try to print
         * the error details.
         */

        if (LLRP::RC_OK != _connectionToReader->sendMessage(sendMsg.get()))
        {
            throw LLRPLaps::ReaderErrorDetailsException(LLRPLaps::ReaderErrorDetailsException::CErrorDetailsToString(_connectionToReader->getSendError(), sendMsg->m_pType->m_pName, "sendMsg"));
        }
    }

    std::string CReader::CErrorDetailsToString(const LLRP::CErrorDetails *errorDetails)
    {
    }


/**
 *****************************************************************************
 **
 ** @brief  Helper to print a message as XML text
 **
 ** Print a LLRP message as XML text
 **
 ** @param[in]  pMessage        Pointer to message to print
 **
 ** @return     void
 **
 *****************************************************************************/

    void CReader::printXMLMessage(std::shared_ptr<LLRP::CMessage> message)
    {
        char aBuf[100 * 1024];
        QString s;

        /*
         * Convert the message to an XML string.
         * This fills the buffer with either the XML string
         * or an error message. The return value could
         * be checked.
         */

        message->toXMLString(aBuf, sizeof aBuf);

        /*
         * Print the XML Text to the standard output.
         */

        // LOG(s.sprintf("%s", aBuf));
    }
}