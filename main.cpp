/*
Copyright (C) 2015 r1ch.net

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define NTDDI_VERSION NTDDI_VISTASP1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <stdint.h>
#include <WinSock2.h>
#include <WS2TCPIP.H>
#include <Ws2ipdef.h>
#include <windows.h>
#include <gdiplus.h>
#include <Wininet.h>
#include <jansson.h>
#include <strsafe.h>
#include <iphlpapi.h>
#include <Tcpestats.h>
#include <commctrl.h>

using namespace Gdiplus;

extern "C" {
#include "librtmp/rtmp.h"
extern uint64_t connectTime;
}

#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

const wchar_t szAppName[] = L"TwitchTest";
HINSTANCE hThisInstance;								// current instance
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
HWND hwndMain;
unsigned int ThreadID;
HANDLE hAbortThreadEvent;

HCURSOR hCursorHand;

// librtmp stuff
#define SAVC(x)    static const AVal av_##x = AVC(#x)
#define STR2AVAL(av,str) av.av_val = str; av.av_len = (int)strlen(av.av_val)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(FCSubscribe);
SAVC(onFCSubscribe);
SAVC(createStream);
SAVC(deleteStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(secureToken);

SAVC(send);

SAVC(onMetaData);
SAVC(duration);
SAVC(width);
SAVC(height);
SAVC(videocodecid);
SAVC(videodatarate);
SAVC(framerate);
SAVC(audiocodecid);
SAVC(audiodatarate);
SAVC(audiosamplerate);
SAVC(audiosamplesize);
SAVC(audiochannels);
SAVC(stereo);
SAVC(encoder);
SAVC(fileSize);

SAVC(onStatus);
SAVC(status);
SAVC(details);
SAVC(clientid);

SAVC(avc1);
SAVC(mp4a);

static const AVal av_NetStream_Play_Start = AVC("NetStream.Play.Start");
static const AVal av_Started_playing = AVC("Started playing");
static const AVal av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
static const AVal av_Stopped_playing = AVC("Stopped playing");

static const AVal av_OBSVersion = AVC("TwitchTest/1.21");
static const AVal av_setDataFrame = AVC("@setDataFrame");

void ProcMainInit (HWND hDlg)
{
	hwndMain = hDlg;

    hCursorHand = LoadCursor(0, IDC_HAND);

    int i;

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"Automatic (OBS)");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, -1);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"System Default");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 0);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"8k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 8192);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"16k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 16384);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"32k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 32768);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"64k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 65536);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"128k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 131072);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"256k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 262144);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"512k");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 524288);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"1M");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 1048576);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"2M");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 1048576 * 2);

    i = SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"4M");
    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETITEMDATA, i, 1048576 * 4);

    SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, 0, 0);

    InvalidateRect(GetDlgItem(hDlg, IDC_GETKEY), NULL, FALSE);

    SendDlgItemMessage(hDlg, IDC_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)0, (LPARAM)LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    SendDlgItemMessage(hDlg, IDC_EU, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_US, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_ASIA, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hDlg, IDC_OTHER, BM_SETCHECK, BST_CHECKED, 0);

	i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"Ping Only");
	SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 0);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"Quick (10 secs)");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 10000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"Medium (30 secs)");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 30000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"Long (60 secs)");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 60000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"2 minutes");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 120000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"3 minutes");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 180000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"4 minutes");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 240000);

    i = SendDlgItemMessage(hDlg, IDC_DURATION, CB_ADDSTRING, 0, (LPARAM)L"5 minutes");
    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETITEMDATA, i, 300000);

    SendDlgItemMessage(hDlg, IDC_DURATION, CB_SETCURSEL, 1, 0);

	DATA_BLOB blobIn;
	DATA_BLOB blobOut;

	BYTE encryptedData[1024];
	DWORD encryptedDataLen = sizeof(encryptedData);

	if (RegGetValue(HKEY_CURRENT_USER, L"SOFTWARE\\r1ch.net\\TwitchTest", L"LastStreamKey", RRF_RT_REG_BINARY, NULL, encryptedData, &encryptedDataLen) == ERROR_SUCCESS)
	{
		blobIn.pbData = (BYTE *)encryptedData;
		blobIn.cbData = encryptedDataLen;

		if (CryptUnprotectData(&blobIn, NULL, NULL, NULL, NULL, 0, &blobOut))
		{
			SetDlgItemText(hwndMain, IDC_EDIT1, (LPCWSTR)blobOut.pbData);
			//SendDlgItemMessage(hwndMain, IDC_EDIT1, EM_SETSEL, -1, 0);
			LocalFree(blobOut.pbData);
		}
	}

    LVCOLUMN col;
    ZeroMemory(&col, sizeof(col));

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    col.fmt = LVCFMT_LEFT;

    col.pszText = L"Server";     col.iOrder = 0; col.cx = 150; ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST), 0, &col);
    col.pszText = L"Bandwidth";  col.iOrder = 1; col.cx = 80; ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST), 1, &col);
    col.pszText = L"RTT";        col.iOrder = 2; col.cx = 60; ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST), 2, &col);
    col.pszText = L"Quality";    col.iOrder = 3; col.cx = 60; ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST), 3, &col);

    SendMessage(hDlg, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

    WSADATA wsad;
    WSAStartup(MAKEWORD(2, 2), &wsad);

    hAbortThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);

    ShowWindow(hDlg, SW_SHOWNORMAL);
}

// FIXME: use the actual OBS socket loop at some point to fully emulate network conditions

/*
void SetupSendBacklogEvent()
{
    zero(&sendBacklogOverlapped, sizeof(sendBacklogOverlapped));

    ResetEvent(hSendBacklogEvent);
    sendBacklogOverlapped.hEvent = hSendBacklogEvent;

    idealsendbacklognotify(rtmp->m_sb.sb_socket, &sendBacklogOverlapped, NULL);
}

void SocketLoop(RTMP *rtmp)
{
    BOOL canWrite = FALSE;

    int delayTime;
    int latencyPacketSize;
    DWORD lastSendTime = 0;

    WSANETWORKEVENTS networkEvents;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    WSAEventSelect(rtmp->m_sb.sb_socket, hWriteEvent, FD_READ | FD_WRITE | FD_CLOSE);

    SetupSendBacklogEvent();

    HANDLE hObjects[3];

    hObjects[0] = hWriteEvent;
    hObjects[1] = hBufferEvent;
    hObjects[2] = hSendBacklogEvent;

    for (;;)
    {
        if (bStopping && WaitForSingleObject(hSocketLoopExit, 0) != WAIT_TIMEOUT)
        {
            OSEnterMutex(hDataBufferMutex);
            if (curDataBufferLen == 0)
            {
                //OSDebugOut (TEXT("Exiting on empty buffer.\n"));
                OSLeaveMutex(hDataBufferMutex);
                break;
            }

            //OSDebugOut (TEXT("Want to exit, but %d bytes remain.\n"), curDataBufferLen);
            OSLeaveMutex(hDataBufferMutex);
        }

        int status = WaitForMultipleObjects(3, hObjects, FALSE, INFINITE);
        if (status == WAIT_ABANDONED || status == WAIT_FAILED)
        {
            Log(TEXT("RTMPPublisher::SocketLoop: Aborting due to WaitForMultipleObjects failure"));
            App->PostStopMessage();
            return;
        }

        if (status == WAIT_OBJECT_0)
        {
            //Socket event
            if (WSAEnumNetworkEvents(rtmp->m_sb.sb_socket, NULL, &networkEvents))
            {
                Log(TEXT("RTMPPublisher::SocketLoop: Aborting due to WSAEnumNetworkEvents failure, %d"), WSAGetLastError());
                App->PostStopMessage();
                return;
            }

            if (networkEvents.lNetworkEvents & FD_WRITE)
                canWrite = true;

            if (networkEvents.lNetworkEvents & FD_CLOSE)
            {
                if (lastSendTime)
                {
                    DWORD diff = OSGetTime() - lastSendTime;
                    Log(TEXT("RTMPPublisher::SocketLoop: Received FD_CLOSE, %u ms since last send (buffer: %d / %d)"), diff, curDataBufferLen, dataBufferSize);
                }

                if (bStopping)
                    Log(TEXT("RTMPPublisher::SocketLoop: Aborting due to FD_CLOSE during shutdown, %d bytes lost, error %d"), curDataBufferLen, networkEvents.iErrorCode[FD_CLOSE_BIT]);
                else
                    Log(TEXT("RTMPPublisher::SocketLoop: Aborting due to FD_CLOSE, error %d"), networkEvents.iErrorCode[FD_CLOSE_BIT]);
                FatalSocketShutdown();
                return;
            }

            if (networkEvents.lNetworkEvents & FD_READ)
            {
                BYTE discard[16384];
                int ret, errorCode;
                BOOL fatalError = FALSE;

                for (;;)
                {
                    ret = recv(rtmp->m_sb.sb_socket, (char *)discard, sizeof(discard), 0);
                    if (ret == -1)
                    {
                        errorCode = WSAGetLastError();

                        if (errorCode == WSAEWOULDBLOCK)
                            break;

                        fatalError = TRUE;
                    }
                    else if (ret == 0)
                    {
                        errorCode = 0;
                        fatalError = TRUE;
                    }

                    if (fatalError)
                    {
                        Log(TEXT("RTMPPublisher::SocketLoop: Socket error, recv() returned %d, GetLastError() %d"), ret, errorCode);
                        FatalSocketShutdown();
                        return;
                    }
                }
            }
        }
        else if (status == WAIT_OBJECT_0 + 2)
        {
            //Ideal send backlog event
            ULONG idealSendBacklog;

            if (!idealsendbacklogquery(rtmp->m_sb.sb_socket, &idealSendBacklog))
            {
                int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);
                if (!getsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
                {
                    if (curTCPBufSize < (int)idealSendBacklog)
                    {
                        int bufferSize = (int)idealSendBacklog;
                        setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof(bufferSize));
                        Log(TEXT("RTMPPublisher::SocketLoop: Increasing send buffer to ISB %d (buffer: %d / %d)"), idealSendBacklog, curDataBufferLen, dataBufferSize);
                    }
                }
                else
                    Log(TEXT("RTMPPublisher::SocketLoop: Got hSendBacklogEvent but getsockopt() returned %d"), WSAGetLastError());
            }
            else
                Log(TEXT("RTMPPublisher::SocketLoop: Got hSendBacklogEvent but WSAIoctl() returned %d"), WSAGetLastError());

            SetupSendBacklogEvent();
            continue;
        }

        if (canWrite)
        {
            bool exitLoop = false;
            do
            {
                OSEnterMutex(hDataBufferMutex);

                if (!curDataBufferLen)
                {
                    //this is now an expected occasional condition due to use of auto-reset events, we could end up emptying the buffer
                    //as it's filled in a previous loop cycle, especially if using low latency mode.
                    OSLeaveMutex(hDataBufferMutex);
                    //Log(TEXT("RTMPPublisher::SocketLoop: Trying to send, but no data available?!"));
                    break;
                }

                int ret;
                if (lowLatencyMode != LL_MODE_NONE)
                {
                    int sendLength = min(latencyPacketSize, curDataBufferLen);
                    ret = send(rtmp->m_sb.sb_socket, (const char *)dataBuffer, sendLength, 0);
                }
                else
                {
                    ret = send(rtmp->m_sb.sb_socket, (const char *)dataBuffer, curDataBufferLen, 0);
                }

                if (ret > 0)
                {
                    if (curDataBufferLen - ret)
                        memmove(dataBuffer, dataBuffer + ret, curDataBufferLen - ret);
                    curDataBufferLen -= ret;

                    bytesSent += ret;

                    if (lastSendTime)
                    {
                        DWORD diff = OSGetTime() - lastSendTime;

                        if (diff >= 1500)
                            Log(TEXT("RTMPPublisher::SocketLoop: Stalled for %u ms to write %d bytes (buffer: %d / %d), unstable connection?"), diff, ret, curDataBufferLen, dataBufferSize);

                        totalSendPeriod += diff;
                        totalSendBytes += ret;
                        totalSendCount++;
                    }

                    lastSendTime = OSGetTime();

                    SetEvent(hBufferSpaceAvailableEvent);
                }
                else
                {
                    int errorCode;
                    BOOL fatalError = FALSE;

                    if (ret == -1)
                    {
                        errorCode = WSAGetLastError();

                        if (errorCode == WSAEWOULDBLOCK)
                        {
                            canWrite = false;
                            OSLeaveMutex(hDataBufferMutex);
                            break;
                        }

                        fatalError = TRUE;
                    }
                    else if (ret == 0)
                    {
                        errorCode = 0;
                        fatalError = TRUE;
                    }

                    if (fatalError)
                    {
                        //connection closed, or connection was aborted / socket closed / etc, that's a fatal error for us.
                        Log(TEXT("RTMPPublisher::SocketLoop: Socket error, send() returned %d, GetLastError() %d"), ret, errorCode);
                        OSLeaveMutex(hDataBufferMutex);
                        FatalSocketShutdown();
                        return;
                    }
                }

                //finish writing for now
                if (curDataBufferLen <= 1000)
                    exitLoop = true;

                OSLeaveMutex(hDataBufferMutex);

                if (delayTime)
                    Sleep(delayTime);
            } while (!exitLoop);
        }
    }

    Log(TEXT("RTMPPublisher::SocketLoop: Graceful loop exit"));
}*/

DWORD GetTcpRow(u_short localPort,u_short remotePort, MIB_TCP_STATE state, __out PMIB_TCPROW row)
{
    PMIB_TCPTABLE tcpTable = NULL;
    PMIB_TCPROW tcpRowIt = NULL;

    DWORD status, size = 0, i;
    BOOL connectionFound = FALSE;

    status = GetTcpTable(tcpTable, &size, TRUE);
    if (status != ERROR_INSUFFICIENT_BUFFER)
        return status;

    tcpTable = (PMIB_TCPTABLE)malloc(size);
    if (tcpTable == NULL)
        return ERROR_OUTOFMEMORY;

    status = GetTcpTable(tcpTable, &size, TRUE);
    if (status != ERROR_SUCCESS)
	{
        free(tcpTable);
        return status;
    }

    for (i = 0; i < tcpTable->dwNumEntries; i++)
	{
        tcpRowIt = &tcpTable->table[i];
        if (tcpRowIt->dwLocalPort == (DWORD)localPort &&
            tcpRowIt->dwRemotePort == (DWORD)remotePort &&
            tcpRowIt->State == state)
		{
            connectionFound = TRUE;
            *row = *tcpRowIt;
            break;
        }
    }

    free(tcpTable);

    if (connectionFound)
        return ERROR_SUCCESS;
    return ERROR_NOT_FOUND;
}

void SendRTMPMetadata(RTMP *rtmp)
{
    char metadata[2048] = {0};
    char *enc = metadata + RTMP_MAX_HEADER_SIZE, *pend = metadata + sizeof(metadata) - RTMP_MAX_HEADER_SIZE;

    enc = AMF_EncodeString(enc, pend, &av_setDataFrame);
    enc = AMF_EncodeString(enc, pend, &av_onMetaData);

    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_width, 16);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_height, 16);
    enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//
    enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, 10000);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, 30);
    enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//audioCodecID);//
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, 128); //ex. 128kb\s
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, 44100);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, 2);
    enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, 1);

    enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &av_OBSVersion);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    RTMPPacket packet = { 0 };

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet.m_packetType = RTMP_PACKET_TYPE_INFO;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = rtmp->m_stream_id;
    packet.m_hasAbsTimestamp = TRUE;
    packet.m_body = metadata + RTMP_MAX_HEADER_SIZE;
    packet.m_nBodySize = enc - metadata + RTMP_MAX_HEADER_SIZE;

    RTMP_SendPacket(rtmp, &packet, FALSE);
}

int jsonServerSort(void * ctx, const void *arg1, const void *arg2)
{
    json_t *ingests = (json_t *)ctx;
    json_t *server;
    json_t *serverName;

    const char *strServerNameA, *strServerNameB;

    server = json_array_get(ingests, *(int *)arg1);
    serverName = json_object_get(server, "name");
    strServerNameA = json_string_value(serverName);

    server = json_array_get(ingests, *(int *)arg2);
    serverName = json_object_get(server, "name");
    strServerNameB = json_string_value(serverName);

    return strcmp(strServerNameA, strServerNameB);
}

void SetupSendEvent(HANDLE hEvent, OVERLAPPED *sendBacklogOverlapped, RTMP *rtmp)
{
    ResetEvent(hEvent);
    ZeroMemory(sendBacklogOverlapped, sizeof(*sendBacklogOverlapped));
    sendBacklogOverlapped->hEvent = hEvent;
    idealsendbacklognotify(rtmp->m_sb.sb_socket, sendBacklogOverlapped, NULL);
}

#define MASK_EU     1
#define MASK_US     2
#define MASK_ASIA   4
#define MASK_OTHER  8

int GetServerMasks(void)
{
    int mask = 0;

    if (SendDlgItemMessage(hwndMain, IDC_EU, BM_GETCHECK, 0, 0) == BST_CHECKED)
        mask += MASK_EU;

    if (SendDlgItemMessage(hwndMain, IDC_US, BM_GETCHECK, 0, 0) == BST_CHECKED)
        mask += MASK_US;

    if (SendDlgItemMessage(hwndMain, IDC_ASIA, BM_GETCHECK, 0, 0) == BST_CHECKED)
        mask += MASK_ASIA;

    if (SendDlgItemMessage(hwndMain, IDC_OTHER, BM_GETCHECK, 0, 0) == BST_CHECKED)
        mask += MASK_OTHER;
    
    return mask;
}

int ServerIsFiltered(const char *name, int mask)
{
    if (!strncmp (name, "EU:", 3))
    {
        if (mask & MASK_EU)
            return 0;
        else
            return 1;
    }
    
    if (!strncmp (name, "US ", 3))
    {
        if (mask & MASK_US)
            return 0;
        else
            return 1;
    }

    if (!strncmp(name, "Asia:", 5))
    {
        if (mask & MASK_ASIA)
            return 0;
        else
            return 1;
    }
    
    if (mask & MASK_OTHER)
        return 0;

    return 1;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

unsigned int __stdcall ScreenshotThread(void *arg)
{
	HDC hDC = GetWindowDC(hwndMain);
	HDC hMemoryDC = CreateCompatibleDC(hDC);
	RECT rect;
	json_t *root = NULL;
	json_error_t error = { 0 };

	SetDlgItemText(hwndMain, IDC_SHARE, L"Uploading...");

	GetWindowRect  (hwndMain, &rect);

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	HBITMAP hBitmap = CreateCompatibleBitmap(hDC, width, height);

	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

	BitBlt(hMemoryDC, 0, 0, width, height, hDC, 0, 0, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

	Bitmap *image = new Bitmap(hBitmap, NULL);
	CLSID myClsId;
	int retVal = GetEncoderClsid(L"image/png", &myClsId);
	if (retVal == -1)
		goto failure;

	wchar_t tempPath[MAX_PATH];
	wchar_t tempFileName[MAX_PATH];

	GetTempPath (_countof(tempPath), tempPath);

	GetTempFileName (tempPath, L"twt", 0, tempFileName);
	image->Save(tempFileName, &myClsId, NULL);
	delete image;

	DeleteDC(hMemoryDC);
	DeleteDC(hDC);

	HANDLE hFile;
	hFile = CreateFile (tempFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		goto failure;

	LARGE_INTEGER fileSize;
	GetFileSizeEx(hFile, &fileSize);

	BYTE *buff = (BYTE *)malloc (fileSize.QuadPart);

	DWORD read;
	ReadFile (hFile, buff, fileSize.QuadPart, &read, nullptr);
	if (read != fileSize.QuadPart)
		goto failure;

	CloseHandle (hFile);

	DeleteFile (tempFileName);

#define BOUNDARY_TEXT "------------TwitchTestIsAwesome"

	HINTERNET hInternet = InternetOpen(L"TwitchTest/1.21", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	HINTERNET hConnect = InternetConnect(hInternet, L"api.imgur.com", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
	if (!hConnect)
		goto failure;

	LPCWSTR acceptTypes[] = { L"*/*", NULL };

	HINTERNET hRequest = HttpOpenRequest (hConnect, L"POST", L"/3/image", L"HTTP/1.1", nullptr, acceptTypes, INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hRequest)
		goto failure;

	char *prefix = "--" BOUNDARY_TEXT "\r\n"
		"Content-Disposition: form-data; name=\"image\"; filename=\"twitchtest2.png\"\r\n"
		"Content-Transfer-Encoding: binary\r\n"
		"Content-Type: image/png\r\n\r\n";

	char *suffix = "\r\n--" BOUNDARY_TEXT "--\r\n";

	DWORD requestLength = (DWORD)fileSize.QuadPart + strlen(prefix) + strlen(suffix);

	BYTE *request = (BYTE *)malloc (requestLength);

	CopyMemory (request, prefix, strlen(prefix));
	CopyMemory (request + strlen(prefix), buff, fileSize.QuadPart);
	CopyMemory (request + strlen(prefix) + fileSize.QuadPart, suffix, strlen(suffix));

	wchar_t headers[1024];

	wsprintf (headers, L"Content-Type: multipart/form-data; boundary=" BOUNDARY_TEXT "\r\nAuthorization: Client-ID " IMGUR_CLIENT_ID "\r\nContent-Length: %u", requestLength);

	int ret = HttpSendRequest(hRequest, headers, 0, request, requestLength);
	free (request);

	if (!ret)
		goto failure;

	char response[16384];
	InternetReadFile (hRequest, response, sizeof(response), &read);
	if (!read)
		goto failure;

	response[read] = 0;
	
	json_t *link;

	const char *url;
	root = json_loads(response, 0, &error);

	if (!json_is_object(root))
		goto failure;

	json_t *data = json_object_get(root, "data");
	if (!data)
		goto failure;

	link = json_object_get(data, "link");
	if (!link)
		goto failure;

	url = json_string_value(link);

	if (!strncmp (url, "https://", 8) && !strncmp (url, "http://", 7))
		goto failure;

	ShellExecuteA(hwndMain, "open", url, NULL, 0, SW_SHOWNORMAL);
	EnableWindow (GetDlgItem(hwndMain, IDC_SHARE), TRUE);
	SetDlgItemText(hwndMain, IDC_SHARE, L"Share Result");

	if (root)
		json_decref(root);

	return 0;

failure:
	if (root)
		json_decref(root);

	MessageBox (hwndMain, L"Uploading image for sharing failed :(", L"Upload Error", MB_ICONERROR);
	EnableWindow(GetDlgItem(hwndMain, IDC_SHARE), TRUE);
	SetDlgItemText(hwndMain, IDC_SHARE, L"Share Result");
	return 1;
}

unsigned int __stdcall BandwidthTest(void *arg)
{
    int i;
    int tcpBufferSize;
    int *indexOrder = NULL;
    RTMP *rtmp;
	json_t *root = NULL;
	json_error_t error = { 0 };
	RTMPPacket packet = { 0 };
	MIB_TCPROW row = { 0 };

    i = SendDlgItemMessage(hwndMain, IDC_COMBO1, CB_GETCURSEL, 0, 0);
    if (i == -1)
    {
        ThreadID = 0;
        goto terribleProblems;
    }

    int serverMasks = GetServerMasks();

	int j = SendDlgItemMessage(hwndMain, IDC_DURATION, CB_GETCURSEL, 0, 0);
	int TEST_DURATION = SendDlgItemMessage(hwndMain, IDC_DURATION, CB_GETITEMDATA, j, 0);

    char keyBuffer[256], *key = NULL;
    keyBuffer[0] = 0;

    SendDlgItemMessageA(hwndMain, IDC_EDIT1, WM_GETTEXT, sizeof(keyBuffer), (LPARAM)keyBuffer);

    if (!keyBuffer[0] && TEST_DURATION)
    {
        ThreadID = 0;
        goto terribleProblems;
    }

    char *p = keyBuffer;
    while (*p && isspace(*p))
        p++;

    key = p;

    while (*p)
    {
        if (isspace(*p))
        {
            *p = 0;
            break;
        }
        p++;
    }

    int len = strlen(keyBuffer) + 1 + 14;
    key = (char *)malloc(len);

    strcpy_s(key, len, keyBuffer);
    strcat_s(key, len, "?bandwidthtest");

    timeBeginPeriod(1);

    HINTERNET hConnect, hInternet;
    char buff[65536];
    int read = 0;
    DWORD ret;

    hInternet = InternetOpen(L"TwitchTest/1.21", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

    hConnect = InternetOpenUrl(hInternet, L"https://api.twitch.tv/kraken/ingests", L"Accept: */*", -1L, INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);

    for (;;)
    {
        if (InternetReadFile(hConnect, buff + read, sizeof(buff) - read, &ret))
        {
            if (ret == 0)
                break;

            read += ret;
        }
        else
        {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            MessageBox(hwndMain, L"Failed to download Twitch ingest list.", L"Error", MB_ICONERROR);
            goto terribleProblems;
        }
    }

    buff[read] = 0;

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    root = json_loads(buff, 0, &error);

    if (!json_is_object(root))
        _endthreadex(2);

    json_t *ingests = json_object_get(root, "ingests");

    SendMessage(hwndMain, WM_APP + 10, json_array_size(ingests), 0);

    indexOrder = (int *)malloc(json_array_size(ingests) * sizeof(int));
    for (i = 0; i < (int)json_array_size(ingests); i++)
        indexOrder[i] = i;

    qsort_s (indexOrder, json_array_size(ingests), sizeof(*indexOrder), jsonServerSort, ingests);

    SendDlgItemMessage(hwndMain, IDC_LIST, LVM_DELETEALLITEMS, 0, 0);
    
    int index = 0;
    for (i = 0; i < (int)json_array_size(ingests); i++)
    {
        json_t *server;
        json_t *serverName;

        const char *strServerName;

        server = json_array_get(ingests, indexOrder[i]);

        serverName = json_object_get(server, "name");
        strServerName = json_string_value(serverName);

        if (ServerIsFiltered(strServerName, serverMasks))
            continue;

        LVITEM lvItem;
        ZeroMemory(&lvItem, sizeof(lvItem));

        lvItem.mask = LVIF_TEXT;

        wchar_t szServerName[256];
        MultiByteToWideChar(CP_UTF8, 0, strServerName, -1, szServerName, _countof(szServerName));

        lvItem.pszText = szServerName;
        lvItem.iSubItem = 0;
        lvItem.lParam = 0;
        lvItem.iItem = index;

        SendDlgItemMessage(hwndMain, IDC_LIST, LVM_INSERTITEM, 0, (LPARAM)(const LPLVITEM)&lvItem);
        index++;
    }

    index = 0;
    for (i = 0; i < (int)json_array_size(ingests); i++)
    {
        json_t *server;
        json_t *serverName;
        json_t *url;

        WSAEVENT hIdealSend = NULL;

        int failed = 0;

        const char *strURL, *strServerName;

        server = json_array_get(ingests, indexOrder[i]);

        serverName = json_object_get(server, "name");
        strServerName = json_string_value(serverName);
        
        if (ServerIsFiltered(strServerName, serverMasks))
            continue;

        url = json_object_get(server, "url_template");
        strURL = json_string_value(url);

        char *myURL = strdup(strURL);
        char *end = strstr(myURL, "/{stream_key}");
        if (!end)
            continue;
        else
            *end = 0;

        ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"Testing...");

        int j = SendDlgItemMessage(hwndMain, IDC_COMBO1, CB_GETCURSEL, 0, 0);
        tcpBufferSize = SendDlgItemMessage(hwndMain, IDC_COMBO1, CB_GETITEMDATA, j, 0);

        rtmp = RTMP_Alloc();

        rtmp->m_inChunkSize = RTMP_DEFAULT_CHUNKSIZE;
        rtmp->m_outChunkSize = RTMP_DEFAULT_CHUNKSIZE;
        rtmp->m_bSendChunkSizeInfo = 1;
        rtmp->m_nBufferMS = 30000;
        rtmp->m_nClientBW = 2500000;
        rtmp->m_nClientBW2 = 2;
        rtmp->m_nServerBW = 2500000;
        rtmp->m_fAudioCodecs = 3191.0;
        rtmp->m_fVideoCodecs = 252.0;
        rtmp->Link.timeout = 30;
        rtmp->Link.swfAge = 30;

        rtmp->Link.flashVer.av_val = "FMLE/3.0 (compatible; FMSc/1.0)";
        rtmp->Link.flashVer.av_len = (int)strlen(rtmp->Link.flashVer.av_val);

        rtmp->m_outChunkSize = 4096;
        rtmp->m_bSendChunkSizeInfo = TRUE;

        RTMP_SetupURL2(rtmp, myURL, key);

        RTMP_EnableWrite(rtmp);
        rtmp->m_bUseNagle = TRUE;

        if (!RTMP_Connect(rtmp, NULL))
        {
            failed = 1;
            goto abortserver;
        }

		wchar_t buff[256];
		StringCbPrintf(buff, sizeof(buff), L"%d ms", (int)connectTime);
		ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 2, buff);

		if (TEST_DURATION == 0)
		{
			RTMP_Close(rtmp);
			closesocket(rtmp->m_sb.sb_socket);

			RTMP_Free(rtmp);

			ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"");

			index++;

			if (WaitForSingleObject(hAbortThreadEvent, 0) == WAIT_OBJECT_0)
				break;

			continue;
		}

        SOCKADDR_STORAGE clientSockName;
        int nameLen = sizeof(SOCKADDR_STORAGE);
        getsockname (rtmp->m_sb.sb_socket, (struct sockaddr *)&clientSockName, &nameLen);
        int clientPort = ((struct sockaddr_in *)(&clientSockName))->sin_port;

        GetTcpRow(clientPort, htons(1935), MIB_TCP_STATE_ESTAB, (PMIB_TCPROW)&row);

        TCP_BOOLEAN_OPTIONAL operation = TcpBoolOptEnabled;
        PUCHAR rw = NULL;
        TCP_ESTATS_DATA_RW_v0 dataOn;
        TCP_ESTATS_PATH_RW_v0 pathOn;
        ULONG size = 0;

        dataOn.EnableCollection = TcpBoolOptEnabled;
        rw = (PUCHAR)&dataOn;
        size = sizeof(dataOn);
        SetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsData, rw, 0, size, 0);

        pathOn.EnableCollection = TcpBoolOptEnabled;
        rw = (PUCHAR)&pathOn;
        size = sizeof(pathOn);
        SetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsPath, rw, 0, size, 0);

        if (!RTMP_ConnectStream(rtmp, 0))
        {
            failed = 1;
            goto abortserver;
        }

        WSAOVERLAPPED sendBacklogOverlapped;

        if (tcpBufferSize > 0)
        {
            setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize));
        }
        else if (tcpBufferSize == -1)
        {
            hIdealSend = WSACreateEvent();
            SetupSendEvent(hIdealSend, &sendBacklogOverlapped, rtmp);

            tcpBufferSize = 65536;
            setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize));
        }

        SendRTMPMetadata(rtmp);

        unsigned char junk[4096] = { 0xde };
		
        packet.m_nChannel = 0x05; // source channel
        packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet.m_body = (char *)junk + RTMP_MAX_HEADER_SIZE;
        packet.m_nBodySize = sizeof(junk) - RTMP_MAX_HEADER_SIZE;

        uint64_t realStart = timeGetTime();
        uint64_t startTime = 0, lastUpdateTime = 0;
        uint64_t bytesSent = 0, startBytes = 0;
        uint64_t sendTime = 0, sendCount = 0;

#define START_MEASUREMENT_WAIT 2500

        float speed = 0;
        int wasCapped = 0;

        for (;;)
        {
            uint64_t nowTime = timeGetTime();

            if (!RTMP_SendPacket(rtmp, &packet, FALSE))
            {
                failed = 1;
                break;
            }

            if (hIdealSend)
            {
                if (WaitForSingleObject(hIdealSend, 0) == WSA_WAIT_EVENT_0)
                {
                    ULONG idealSendBacklog;

                    if (!idealsendbacklogquery(rtmp->m_sb.sb_socket, &idealSendBacklog))
                    {
                        int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);
                        if (!getsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
                        {
                            if (curTCPBufSize < (int)idealSendBacklog)
                            {
                                int bufferSize = (int)idealSendBacklog;
                                setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof(bufferSize));
                            }
                        }
                    }
                    SetupSendEvent(hIdealSend, &sendBacklogOverlapped, rtmp);
                }
            }

            uint64_t diff = timeGetTime() - nowTime;
            
            sendTime += diff;
            sendCount++;

            bytesSent += packet.m_nBodySize;

            if (nowTime - realStart > START_MEASUREMENT_WAIT && !startTime)
            {
                startTime = nowTime;
                startBytes = bytesSent;
                lastUpdateTime = nowTime;
            }

            if (bytesSent - startBytes > 0 && nowTime - startTime > 0)
            {
                speed = ((bytesSent - startBytes)) / ((nowTime - startTime) / 1000.0f);
                speed = speed * 8 / 1000;
                if (speed > 10000)
                {
                    wasCapped = 1;
                    Sleep(10);
                }
            }

            if (startTime && nowTime - lastUpdateTime > 500)
            {
                wchar_t buff[256];
                lastUpdateTime = nowTime;

                if (wasCapped)
                {
                    ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"10000+ kbps");
                }
                else
                {
                    StringCbPrintf (buff, sizeof(buff), L"%d kbps", (int)speed);
                    ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, buff);
                }

                if (nowTime - startTime > TEST_DURATION)
                    break;

                wasCapped = 0;
            }

            if (WaitForSingleObject(hAbortThreadEvent, 0) == WAIT_OBJECT_0)
            {
                ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"Aborted.");
                break;
            }
        }

        if (!failed)
        {
            TCP_ESTATS_DATA_ROD_v0 statsInfo = { 0 };
            TCP_ESTATS_PATH_ROD_v0 pathInfo = { 0 };

            GetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsData, NULL, 0, 0, 0, 0, 0, (PUCHAR)&statsInfo, 0, sizeof(statsInfo));
            GetPerTcpConnectionEStats((PMIB_TCPROW)&row, TcpConnectionEstatsPath, NULL, 0, 0, 0, 0, 0, (PUCHAR)&pathInfo, 0, sizeof(pathInfo));

            /*wchar_t buff[256];
            StringCbPrintf(buff, sizeof(buff), L"%d ms", (int)pathInfo.SmoothedRtt);
            ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 2, buff);*/

            int lost = pathInfo.FastRetran + pathInfo.PktsRetrans;

            lost = 100 - lost - (sendTime / sendCount) * 4;
            if (lost < 0)
                lost = 0;

            StringCbPrintf(buff, sizeof(buff), L"%d", lost);
            ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 3, buff);
        }
        else
        {
            ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"Failed.");
        }

    abortserver:

        if (failed)
            ListView_SetItemText(GetDlgItem(hwndMain, IDC_LIST), index, 1, L"Failed.");

        RTMP_DeleteStream(rtmp);

        shutdown(rtmp->m_sb.sb_socket, SD_SEND);

        //this waits for the socket shutdown to complete gracefully
        for (;;)
        {
            char buff[1024];
            int ret;

            ret = recv(rtmp->m_sb.sb_socket, buff, sizeof(buff), 0);
            if (!ret)
                break;
            else if (ret == -1)
                break;
        }

        RTMP_Close(rtmp);
        closesocket(rtmp->m_sb.sb_socket);

        RTMP_Free(rtmp);

        if (hIdealSend)
            CloseHandle(hIdealSend);

        free(myURL);

        index++;

        if (WaitForSingleObject(hAbortThreadEvent, 0) == WAIT_OBJECT_0)
            break;
    }

terribleProblems:

	if (root)
		json_decref(root);

    if (indexOrder)
        free(indexOrder);

    if (key)
        free(key);

    timeEndPeriod(1);

    SetDlgItemText(hwndMain, IDOK, L"Start");
    ThreadID = 0;

	EnableWindow(GetDlgItem(hwndMain, IDC_SHARE), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_US), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_EU), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_ASIA), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_OTHER), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_EDIT1), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_COMBO1), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDC_DURATION), TRUE);
    EnableWindow(GetDlgItem(hwndMain, IDOK), TRUE);
    return 0;
}

VOID SaveSettings(VOID)
{
	DWORD	result;
	HKEY	hk;
	wchar_t	temp[256];

	RegCreateKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\r1ch.net\\TwitchTest", 0, NULL, 0, KEY_WRITE, NULL, &hk, &result);

	if (!(RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\r1ch.net\\TwitchTest", 0, KEY_WRITE, &hk)))
	{
		DATA_BLOB blobIn;
		DATA_BLOB blobOut;

		GetDlgItemText(hwndMain, IDC_EDIT1, temp, _countof(temp) - 1);
		
		blobIn.pbData = (BYTE *)temp;
		blobIn.cbData = (wcslen(temp) + 1) * sizeof(wchar_t);

		if (CryptProtectData(&blobIn, NULL, NULL, NULL, NULL, 0, &blobOut))
		{
			RegSetValueEx(hk, L"LastStreamKey", 0, REG_BINARY, (const BYTE *)blobOut.pbData, blobOut.cbData);
			LocalFree(blobOut.pbData);
		}

		RegCloseKey (hk);
	}
}

LRESULT CALLBACK ProcMain(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			ProcMainInit(hDlg);
			return FALSE;

		//case WM_QUERYUISTATE:
			//return UISF_HIDEFOCUS;

        case WM_CHANGEUISTATE:
        {
            // Disable focus rectangles by setting or masking out the flag where appropriate. 
            switch (LOWORD(wParam))
            {
            case UIS_SET:
                wParam |= UISF_HIDEFOCUS << 16;
                break;
            case UIS_CLEAR:
            case UIS_INITIALIZE:
                wParam &= ~(UISF_HIDEFOCUS << 16);
                break;
            }
        }
        break;

		case WM_COMMAND :
			switch (LOWORD(wParam))
			{
				case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (ThreadID)
                        {
                            SetEvent(hAbortThreadEvent);
                            EnableWindow(GetDlgItem(hwndMain, IDOK), FALSE);
                            return TRUE;
                        }

						int j = SendDlgItemMessage(hwndMain, IDC_DURATION, CB_GETCURSEL, 0, 0);
						int TEST_DURATION = SendDlgItemMessage(hwndMain, IDC_DURATION, CB_GETITEMDATA, j, 0);

                        if (SendDlgItemMessageA(hwndMain, IDC_EDIT1, WM_GETTEXTLENGTH, 0, 0) == 0 && TEST_DURATION > 0)
                        {
                            MessageBox(hDlg, L"Please enter your Twitch stream key.", L"Error", MB_ICONEXCLAMATION);
                        }
                        else if (GetServerMasks() == 0)
                        {
                            MessageBox(hDlg, L"Please choose at least one region to test.", L"Error", MB_ICONEXCLAMATION);
                        }
                        else
                        {
                            ResetEvent(hAbortThreadEvent);
                            SetDlgItemText(hwndMain, IDOK, L"Abort");
                            //EnableWindow(GetDlgItem(hwndMain, IDOK), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_US), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_EU), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_ASIA), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_OTHER), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_EDIT1), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_COMBO1), FALSE);
                            EnableWindow(GetDlgItem(hwndMain, IDC_DURATION), FALSE);
							EnableWindow(GetDlgItem(hwndMain, IDC_SHARE), FALSE);
					        _beginthreadex (NULL, 0, (_beginthreadex_proc_type)BandwidthTest, NULL, 0, &ThreadID);
                        }
                    }
					return TRUE;

				case IDC_SHARE:
					unsigned int dummy;
					EnableWindow(GetDlgItem(hwndMain, IDC_SHARE), FALSE);
					_beginthreadex(NULL, 0, (_beginthreadex_proc_type)ScreenshotThread, NULL, 0, &dummy);
					return TRUE;

				case IDC_EXIT:
                    if (HIWORD(wParam) == BN_CLICKED)
					    DestroyWindow (hwndMain);
					return TRUE;
                case IDC_GETKEY:
                    if (HIWORD(wParam) == BN_CLICKED)
                        ShellExecute(hDlg, L"open", L"http://www.twitch.tv/broadcast/dashboard/streamkey", NULL, NULL, SW_SHOWNORMAL);
			}
			break;
        case WM_SETCURSOR:
            if ((HWND)wParam == GetDlgItem(hDlg, IDC_GETKEY))
            {
                SetCursor(hCursorHand);
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            return FALSE;

        case WM_CTLCOLORSTATIC:
            {
                if ((HWND)lParam == GetDlgItem(hDlg, IDC_GETKEY))
                {
                    HDC hdcStatic = (HDC)wParam;
                    SetBkMode (hdcStatic, TRANSPARENT);
                    SetTextColor(hdcStatic, RGB(0, 0, 255));
                    return (LRESULT)GetSysColorBrush(COLOR_MENU);
                }
            }
            break;

		case WM_DESTROY:
			SaveSettings();
			PostQuitMessage (0);
			return TRUE;

		case WM_CLOSE:
			DestroyWindow (hwndMain);
			return TRUE;
	}

	return FALSE;
}

//
// Entry point
//
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpszCmdLine, int nCmdShow)
{
	HWND Myhwnd;
	MSG	  msg;
	HICON hIcon;
    INITCOMMONCONTROLSEX	common;

    hThisInstance = hInstance;

    common.dwSize = sizeof(common);
    common.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;

    InitCommonControlsEx(&common);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	//create window
	Myhwnd = CreateDialog (hThisInstance, MAKEINTRESOURCE(IDD_DIALOG1), 0, (DLGPROC)ProcMain);

	if (!Myhwnd)
	{
		MessageBox(NULL, L"Unable to create initial window. This is probably a good time to reboot.", szAppName, MB_OK);
		return 1;
	}

	//set icon
	hIcon = (HICON)LoadImage(	hThisInstance,
								MAKEINTRESOURCE(IDI_ICON2),
								IMAGE_ICON,
								GetSystemMetrics(SM_CXSMICON),
								GetSystemMetrics(SM_CYSMICON),
								0);

	if(hIcon)
		SendMessage(hwndMain, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!IsDialogMessage(hwndMain, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	GdiplusShutdown(gdiplusToken);

	return (int)(msg.wParam);
}
