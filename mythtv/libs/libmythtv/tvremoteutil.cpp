#include <unistd.h>

#include <QFile>

#include "tvremoteutil.h"
#include "cardutil.h"
#include "inputinfo.h"
#include "programinfo.h"
#include "mythcontext.h"
#include "decodeencode.h"
#include "remoteencoder.h"
#include "tv_rec.h"

uint RemoteGetFlags(uint cardid)
{
    if (gContext->IsBackend())
    {
        const TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
            return rec->GetFlags();
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "GET_FLAGS";
    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return 0;

    return strlist[0].toInt();
}

uint RemoteGetState(uint cardid)
{
    if (gContext->IsBackend())
    {
        const TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
            return rec->GetState();
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "GET_STATE";
    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return kState_ChangingState;

    return strlist[0].toInt();
}


bool RemoteRecordPending(uint cardid, const ProgramInfo *pginfo,
                         int secsleft, bool hasLater)
{
    if (gContext->IsBackend())
    {
        TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
        {
            rec->RecordPending(pginfo, secsleft, hasLater);
            return true;
        }
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "RECORD_PENDING";
    strlist << QString::number(secsleft);
    strlist << QString::number(hasLater);
    pginfo->ToStringList(strlist);

    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return false;

    return strlist[0].toUpper() == "OK";
}

bool RemoteStopLiveTV(uint cardid)
{
    if (gContext->IsBackend())
    {
        TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
        {
            rec->StopLiveTV();
            return true;
        }
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "STOP_LIVETV";

    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return false;

    return strlist[0].toUpper() == "OK";
}

bool RemoteStopRecording(uint cardid)
{
    if (gContext->IsBackend())
    {
        TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
        {
            rec->StopRecording();
            return true;
        }
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "STOP_RECORDING";

    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return false;

    return strlist[0].toUpper() == "OK";
}

void RemoteStopRecording(const ProgramInfo *pginfo)
{
    QStringList strlist(QString("STOP_RECORDING"));
    pginfo->ToStringList(strlist);

    gContext->SendReceiveStringList(strlist);
}

void RemoteCancelNextRecording(uint cardid, bool cancel)
{
    QStringList strlist(QString("QUERY_RECORDER %1").arg(cardid));
    strlist << "CANCEL_NEXT_RECORDING";
    strlist << QString::number((cancel) ? 1 : 0);

    gContext->SendReceiveStringList(strlist);
}

RemoteEncoder *RemoteRequestNextFreeRecorder(int curr)
{
    QStringList strlist( "GET_NEXT_FREE_RECORDER" );
    strlist << QString("%1").arg(curr);

    if (!gContext->SendReceiveStringList(strlist, true))
        return NULL;

    int num = strlist[0].toInt();
    QString hostname = strlist[1];
    int port = strlist[2].toInt();

    return new RemoteEncoder(num, hostname, port);
}

/**
 * Given a list of recorder numbers, return the first available.
 */
RemoteEncoder *RemoteRequestFreeRecorderFromList(
    const QStringList &qualifiedRecorders)
{
    QStringList strlist( "GET_FREE_RECORDER_LIST" );

    if (!gContext->SendReceiveStringList(strlist, true))
        return NULL;

    for (QStringList::const_iterator recIter = qualifiedRecorders.begin();
         recIter != qualifiedRecorders.end(); ++recIter)
    {
        if (!strlist.contains(*recIter))
        {
            // did not find it in the free recorder list. We
            // move on to check the next recorder
            continue;
        }
        // at this point we found a free recorder that fulfills our request
        return RemoteGetExistingRecorder((*recIter).toInt());
    }
    // didn't find anything. just return NULL.
    return NULL;
}

RemoteEncoder *RemoteRequestRecorder(void)
{
    QStringList strlist( "GET_FREE_RECORDER" );

    if (!gContext->SendReceiveStringList(strlist, true))
        return NULL;

    int num = strlist[0].toInt();
    QString hostname = strlist[1];
    int port = strlist[2].toInt();

    return new RemoteEncoder(num, hostname, port);
}

RemoteEncoder *RemoteGetExistingRecorder(const ProgramInfo *pginfo)
{
    QStringList strlist( "GET_RECORDER_NUM" );
    pginfo->ToStringList(strlist);

    if (!gContext->SendReceiveStringList(strlist))
        return NULL;

    int num = strlist[0].toInt();
    QString hostname = strlist[1];
    int port = strlist[2].toInt();

    return new RemoteEncoder(num, hostname, port);
}

RemoteEncoder *RemoteGetExistingRecorder(int recordernum)
{
    QStringList strlist( "GET_RECORDER_FROM_NUM" );
    strlist << QString("%1").arg(recordernum);

    if (!gContext->SendReceiveStringList(strlist))
        return NULL;

    QString hostname = strlist[0];
    int port = strlist[1].toInt();

    return new RemoteEncoder(recordernum, hostname, port);
}

vector<InputInfo> RemoteRequestFreeInputList(
    uint cardid, const vector<uint> &excluded_cardids)
{
    vector<InputInfo> list;

    QStringList strlist(QString("QUERY_RECORDER %1").arg(cardid));
    strlist << "GET_FREE_INPUTS";
    for (uint i = 0; i < excluded_cardids.size(); i++)
        strlist << QString::number(excluded_cardids[i]);

    if (!gContext->SendReceiveStringList(strlist))
        return list;

    QStringList::const_iterator it = strlist.begin();
    if ((it == strlist.end()) || (*it == "EMPTY_LIST"))
        return list;

    while (it != strlist.end())
    {
        InputInfo info;
        if (!info.FromStringList(it, strlist.end()))
            break;
        list.push_back(info);
    }

    return list;
}

InputInfo RemoteRequestBusyInputID(uint cardid)
{
    InputInfo blank;

    QStringList strlist(QString("QUERY_RECORDER %1").arg(cardid));
    strlist << "GET_BUSY_INPUT";

    if (!gContext->SendReceiveStringList(strlist))
        return blank;

    QStringList::const_iterator it = strlist.begin();
    if ((it == strlist.end()) || (*it == "EMPTY_LIST"))
        return blank;

    InputInfo info;
    if (info.FromStringList(it, strlist.end()))
        return info;

    return blank;
}

void RemoteSendMessage(const QString &message)
{
    QStringList strlist( "MESSAGE" );
    strlist << message;

    gContext->SendReceiveStringList(strlist);
}

void RemoteGeneratePreviewPixmap(ProgramInfo *pginfo)
{
    QStringList strlist( "QUERY_GENPIXMAP" );
    pginfo->ToStringList(strlist);

    gContext->SendReceiveStringList(strlist);
}

bool RemoteIsBusy(uint cardid, TunedInputInfo &busy_input)
{
    //VERBOSE(VB_IMPORTANT, QString("RemoteIsBusy(%1) %2")
    //        .arg(cardid).arg(gContext->IsBackend() ? "be" : "fe"));

    busy_input.Clear();

    if (gContext->IsBackend())
    {
        const TVRec *rec = TVRec::GetTVRec(cardid);
        if (rec)
            return rec->IsBusy(&busy_input);
    }

    QStringList strlist(QString("QUERY_REMOTEENCODER %1").arg(cardid));
    strlist << "IS_BUSY";
    if (!gContext->SendReceiveStringList(strlist) || strlist.empty())
        return true;

    QStringList::const_iterator it = strlist.begin();
    bool state = (*it).toInt();
    it++;
    if (!busy_input.FromStringList(it, strlist.end()))
        state = true; // if there was an error pretend that the input is busy.

    return state;
}

bool RemoteGetRecordingStatus(
    vector<TunerStatus> *tunerList, bool list_inactive)
{
    bool isRecording = false;
    vector<uint> cardlist = CardUtil::GetCardList();

    if (tunerList)
        tunerList->clear();

    for (uint i = 0; i < cardlist.size(); i++)
    {
        QString     status      = "";
        uint        cardid      = cardlist[i];
        int         state       = kState_ChangingState;
        QString     channelName = "";
        QString     title       = "";
        QString     subtitle    = "";
        QDateTime   dtStart     = QDateTime();
        QDateTime   dtEnd       = QDateTime();
        QStringList strlist;

        QString cmd = QString("QUERY_REMOTEENCODER %1").arg(cardid);

        while (state == kState_ChangingState)
        {
            strlist = QStringList(cmd);
            strlist << "GET_STATE";
            gContext->SendReceiveStringList(strlist);

            if (strlist.empty())
                break;

            state = strlist[0].toInt();
            if (kState_ChangingState == state)
                usleep(5000);
        }

        if (kState_RecordingOnly == state || kState_WatchingRecording == state)
        {
            isRecording |= true;

            if (!tunerList)
                break;

            strlist = QStringList(QString("QUERY_RECORDER %1").arg(cardid));
            strlist << "GET_RECORDING";
            gContext->SendReceiveStringList(strlist);

            ProgramInfo progInfo;
            QStringList::const_iterator it = strlist.constBegin();
            progInfo.FromStringList(it, strlist.constEnd());

            title       = progInfo.title;
            subtitle    = progInfo.subtitle;
            channelName = progInfo.channame;
            dtStart     = progInfo.startts;
            dtEnd       = progInfo.endts;
        }
        else if (!list_inactive)
            continue;

        if (tunerList)
        {
            TunerStatus tuner;
            tuner.id          = cardid;
            tuner.isRecording = ((kState_RecordingOnly     == state) ||
                                  (kState_WatchingRecording == state));
            tuner.channame    = channelName;
            tuner.title       = (kState_ChangingState == state) ?
                QObject::tr("Error querying recorder state") : title;
            tuner.subtitle    = subtitle;
            tuner.startTime   = dtStart;
            tuner.endTime     = dtEnd;
            tunerList->push_back(tuner);
        }
    }

    return isRecording;
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
