/* //vendor/sprd/proprietories-source/ril/sprd_libril/RilATCISocket.cpp
 *
 * AT Command Interface Server Socket implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 */

#ifndef RIL_ATCI_SOCKET_H_INCLUDED
#define RIL_ATCI_SOCKET_H_INCLUDED

#include "telephony/sprd_ril.h"
#include "RilSocket.h"

/**
 * RilATCISocket is a derived class, derived from the RilSocket abstract
 * class, representing sockets for communication between sprd_atci.cpp and
 * the ril daemon.
 * <p>
 * This class performs the following functions :
 * <ul>
 *     <li>Initialize the socket.
 *     <li>Process the requests coming on the socket.
 *     <li>Provide handlers for request responses.
 *     <li>Request and pending response queue handling.
 * </ul>
 */
class RilATCISocket : public RilSocket {
    /**
     * Function pointer to the ril initialization funtion.
     *
     * @param Ril environment variable with place request and
     *        response handlers and timeout handler.
     *
     * @param Number of arguements for the initialization function.
     *
     * @param Arguements to the initialization function used to
     *        generate instance id of the ril daemon.
     *
     * @return Radio functions with handlers for onRequest, onStateRequest,
     *         supports, onCancel and getVersion.
     */
    RIL_RadioFunctions *(*ATCIInit)(const struct RIL_Env *, int argc, char **argv);

    /**
     * Place holder for the radio functions returned by the initialization
     * function. Currenty only onRequest handler is being used.
     */
    RIL_RadioFunctions* atciFuncs;

    /**
     * Wrapper struct for handling the requests in the queue.
     */
    typedef struct ATCISocketRequest {
        int token;
        int request;
        char *data;
        struct ATCISocketRequest* p_next;
        RIL_SOCKET_ID socketId;
    } ATCISocketRequest;

    /**
     * Queue for requests that are pending dispatch.
     */
    Ril_queue<ATCISocketRequest> dispatchQueue;

    /**
     * Queue for requests that are dispatched but are pending response
     */
    Ril_queue<ATCISocketRequest> pendingResponseQueue;

    public:
        /**
         * Initialize the socket and add the socket to the list.
         *
         * @param Name of the socket.
         * @param Radio functions to be used by the socket.
         */
        static void initATCISocket(const char *socketName,
        RIL_RadioFunctions *atciFuncs);

        /**
         * Process requests from the dispatch request queue.
         * @param Request to be dispatched.
         */
        int processRequest(ATCISocketRequest *request);

        /**
         * Ril envoronment variable that holds the request and
         * unsol response handlers.
         */
        static struct RIL_Env atciRilEnv;

        /**
         * Function to print the socket list.
         */
        static void printList();

        /**
         * Clean up method to be called on command close.
         */
        void onCommandsSocketClosed(void);

        /**
         * Datatype to handle the socket list.
         */
        typedef struct RilATCISocketList {
            RilATCISocket* socket;
            RilATCISocketList *next;
        } RilATCISocketList;

    protected:
        /**
         * Process each record read from the socket and
         * push a new request created from that record to
         * the dispatch request queue.
         *
         * @param The record data.
         * @param The record length.
         */
        void pushRecord(void *record, size_t recordlen);

        /**
         * Socket handler to be called when a request has
         * been completed.
         *
         * @param Token associated with the request.
         * @param Error, if any, while processing the request.
         * @param The response payload.
         * @param Response payload length.
         */
        void onRequestComplete(RIL_Token t,RIL_Errno e,
        void *response, size_t response_len);

        /**
         * Class method to get the socket from the socket list.
         *
         * @param Socket id.
         * @return the at channel socket.
         */
        static RilATCISocket* getSocketById(RIL_SOCKET_ID socketId);

        /**
         * Method to send response to sprd_atci.cpp. It does an atomic write operation on the
         * socket.
         *
         * @param the response header with the payload.
         */
        void sendResponse(const void *data, size_t dataSize);

        /**
         * A loop for processing the requests in the request dispatch queue.
         */
        void *processRequestsLoop(void);

        /**
         * Class method to add the at command interface socket to the list of sockets.
         * Does nothing if the socket is already present in the list.
         * Otherwise, calls the constructor of the parent class(To startlistening)
         * and add socket to the socket list.
         */
        static void addSocketToList(const char *socketName, RIL_SOCKET_ID socketid,
        RIL_RadioFunctions *atciFuncs);

        /**
         * Check if a socket of the given name exists in the socket list.
         *
         * @param Socket name.
         * @return true if exists, false otherwise.
         */
        static bool SocketExists(const char *socketName);

    private:
        /**
         * Constructor.
         *
         * @param Socket name.
         * @param Socket id.
         * @param Radio functions.
         */
        RilATCISocket(const char *socketName,
        RIL_SOCKET_ID socketId,
        RIL_RadioFunctions *inputATCIFuncs);

        /**
         * Called by the processRequest method to dispatch the request to
         * the lower layers. It calls the on request function.
         *
         * @param The request message.
         */
        void dispatchRequest(ATCISocketRequest *request);

        /**
         * Class method that selects the socket on which the onRequestComplete
         * is called.
         *
         * @param Token associated with the request.
         * @param Error, if any, while processing the request.
         * @param The response payload.
         * @param Response payload length.
         */
        static void sOnRequestComplete(RIL_Token t,
        RIL_Errno e, void *response, size_t responselen);
};

#endif /*RIL_UIM_SOCKET_H_INCLUDED*/
