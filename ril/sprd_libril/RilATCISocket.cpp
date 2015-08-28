/* //vendor/sprd/proprietories-source/ril/sprd_libril/RilATCISocket.cpp
 *
 * AT Command Interface Server Socket implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 */

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <utils/Log.h>
#include <arpa/inet.h>
#include <errno.h>
#include <binder/Parcel.h>
#include <cutils/jstring.h>
#include "telephony/sprd_ril.h"
#include "RilATCISocket.h"

using namespace android;

#define LOG_TAG "RIL_ATCI_SOCKET"

RilATCISocket::RilATCISocketList *head;

extern "C" void writeStringToParcel(Parcel &p, const char *s);
extern "C" char * strdupReadString(Parcel &p);
struct RIL_Env RilATCISocket::atciRilEnv = {
        .OnRequestComplete = RilATCISocket::sOnRequestComplete,
};

void RilATCISocket::sOnRequestComplete (RIL_Token t,
        RIL_Errno e,
        void *response,
        size_t responselen) {

    RilATCISocket *atciSocket;
    ATCISocketRequest *request = (ATCISocketRequest*) t;

    RLOGD("Socket id:%d", request->socketId);

    atciSocket = getSocketById(request->socketId);

    if (atciSocket) {
        atciSocket->onRequestComplete(t,e,response,responselen);
    } else {
        RLOGE("Invalid socket id");
        free(request->data);
        free(request);
    }
}

void RilATCISocket::printList() {
    RilATCISocketList *current = head;
    RLOGD("Printing socket list");
    while(NULL != current) {
        RLOGD("SocketName:%s",(current->socket->name == NULL)? "NULL" : current->socket->name);
        RLOGD("Socket id:%d",current->socket->id);
        current = current->next;
    }
}

RilATCISocket *RilATCISocket::getSocketById(RIL_SOCKET_ID socketId) {
    RilATCISocket *atciSocket;
    RilATCISocketList *current = head;

    printList();

    while(NULL != current) {
        if(socketId == current->socket->id) {
            atciSocket = current->socket;
            return atciSocket;
        }
        current = current->next;
    }
    return NULL;
}

void RilATCISocket::initATCISocket(const char *socketName,
        RIL_RadioFunctions *atciFuncs) {

    if (strcmp(socketName, "atci_socket1") == 0) {
        if(!SocketExists(socketName)) {
            addSocketToList(socketName, RIL_SOCKET_1, atciFuncs);
        }
    } else if (strcmp(socketName, "atci_socket2") == 0) {
        if(!SocketExists(socketName)) {
            addSocketToList(socketName, RIL_SOCKET_2, atciFuncs);
        }
    } else if (strcmp(socketName, "atci_socket3") == 0) {
        if(!SocketExists(socketName)) {
            addSocketToList(socketName, RIL_SOCKET_3, atciFuncs);
        }
    } else if (strcmp(socketName, "atci_socket4") == 0) {
        if(!SocketExists(socketName)) {
            addSocketToList(socketName, RIL_SOCKET_4, atciFuncs);
        }
    }
}

void RilATCISocket::addSocketToList(const char *socketName, RIL_SOCKET_ID socketid,
        RIL_RadioFunctions *atciFuncs) {
    RilATCISocket* socket = NULL;
    RilATCISocketList* listItem = (RilATCISocketList*)malloc(sizeof(RilATCISocketList));
    RilATCISocketList *current;

    if(!SocketExists(socketName)) {
        socket = new RilATCISocket(socketName, socketid, atciFuncs);
        listItem->socket = socket;
        listItem->next = NULL;

        RLOGD("Adding socket with id: %d", socket->id);

        if(NULL == head) {
            head = listItem;
            head->next = NULL;
        } else {
            current = head;
            while(NULL != current->next) {
                current = current->next;
            }
            current->next = listItem;
        }
        socket->socketInit();
    }
}

bool RilATCISocket::SocketExists(const char *socketName) {
    RilATCISocketList* current = head;

    while(NULL != current) {
        if(strcmp(current->socket->name, socketName) == 0) {
            return true;
        }
        current = current->next;
    }
    return false;
}

void* RilATCISocket::processRequestsLoop(void) {
    ATCISocketRequest *req = NULL;
    RLOGI("ATCI_SOCKET:Request loop started");

    while(true) {
        req = dispatchQueue.dequeue();

        RLOGI("New request from the dispatch Queue");

        if (req != NULL) {
            processRequest(req);
            free(req);
        } else {
            RLOGE("Fetched null buffer from queue!");
        }
    }
    return NULL;
}

RilATCISocket::RilATCISocket(const char *socketName,
        RIL_SOCKET_ID socketId,
        RIL_RadioFunctions *inputATCIFuncs):
        RilSocket(socketName, socketId) {
    if (inputATCIFuncs) {
        atciFuncs = inputATCIFuncs;
    }
}

int RilATCISocket::processRequest(ATCISocketRequest *request) {
    dispatchRequest(request);
    return 0;
}

void RilATCISocket::dispatchRequest(ATCISocketRequest *req) {
    ATCISocketRequest* currRequest=(ATCISocketRequest*)malloc(sizeof(ATCISocketRequest));

    currRequest->token = req->token;
    currRequest->request = req->request;
    currRequest->data = req->data;
    currRequest->p_next = NULL;
    currRequest->socketId = id;

    pendingResponseQueue.enqueue(currRequest);

    if (atciFuncs) {
        RLOGI("> AT REQUEST socketId: %d. data: %s",
        req->socketId,
        (req->data == NULL)? "NULL":req->data);

        atciFuncs->onRequest(req->request, req->data, strlen(req->data)+1, req);
    }
}

void RilATCISocket::onRequestComplete(RIL_Token t, RIL_Errno e, void *response,
        size_t response_len) {
    ATCISocketRequest* req= (ATCISocketRequest*)t;

    if (req->data) {
        Parcel p;
        p.writeInt32 (req->token);
        p.writeInt32 (e);

        if (response == NULL) {
            p.writeInt32 (0);
        } else {
            int numStrings;
            char **p_cur = (char **) response;

            numStrings = response_len / sizeof(char *);
            p.writeInt32 (numStrings);
            for (int i = 0 ; i < numStrings ; i++) {
                writeStringToParcel (p, p_cur[i]);
            }
        }

        if(!pendingResponseQueue.checkAndDequeue(req->request, req->token)) {
            RLOGE("Request:%d, Token:%d", req->request, req->token);
            return;
        }

        sendResponse(p.data(), p.dataSize());
        free(req->data);
    }
}

void RilATCISocket::sendResponse(const void *data, size_t dataSize) {
    pthread_mutex_lock(&write_lock);
    int ret;
    uint32_t header;

    header = htonl(dataSize);

    ret = blockingWrite_helper(commandFd, (void *)&header, sizeof(header));
    if (ret < 0) {
        RLOGE("ATCI Server:  blockingWrite header error");
        goto ERROR;
    }

    ret = blockingWrite_helper(commandFd, data, dataSize);
    if (ret < 0) {
        RLOGE("ATCI Server:  blockingWrite header error");
        goto ERROR;
    }

ERROR:
    pthread_mutex_unlock(&write_lock);
}

void RilATCISocket::pushRecord(void *p_record, size_t recordlen) {
    Parcel p;
    int request;
    int token;

    p.setData((uint8_t *) p_record, recordlen);
    p.readInt32(&token);
    p.readInt32(&request);

    ATCISocketRequest *recv = (ATCISocketRequest*)malloc(sizeof(ATCISocketRequest));

    recv->token = token;
    recv->request = request;
    recv->socketId = id;
    char *data = NULL;
    data = strdupReadString(p);
    recv->data = data;

    dispatchQueue.enqueue(recv);
}

void RilATCISocket::onCommandsSocketClosed() {
    RLOGE("Socket command closed");
}
