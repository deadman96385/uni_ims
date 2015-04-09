/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2004 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev$ $Date$
 */

#include "osal_ut.h"
/* the port number can be changed, if necessary. */
#define NET_UT_SERVER_PORT 10000
#define NET_UT_CLIENT_PORT 5001
#define NET_UT_BUF_SIZE 64
/* ssl server ip can be changed, if necessary. */
#define NET_UT_SSL_SERVER "122.201.206.179"

char    net_ut_clientSendBuf[NET_UT_BUF_SIZE];
char    net_ut_serverSendBuf[NET_UT_BUF_SIZE];
int net_ut_server_completed;
int net_ut_client_completed;
OSAL_NetAddress net_ut_serverAddr;
OSAL_NetAddress net_ut_clientAddr;
OSAL_NetSslId   net_ut_sslServer;
OSAL_NetSslId   net_ut_sslClient;
/*
 * ======== _OSALUT_netTcpServer() ========
 *
 * Function to create TCP server socket, receive data from client and check
 * if the data is correct.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netTcpServer(
    OSAL_NetSockType  type)
{
    OSAL_NetSockId socketFd;
    OSAL_NetSockId  connSocket;
    int size;
    char    buf[NET_UT_BUF_SIZE];
    static int ret = OSAL_FAIL;


    /* Create server socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("%s:%d Cannot create TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Bind server socket. */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Cannot bind TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    if (OSAL_SUCCESS != OSAL_netListenOnSocket(&socketFd)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to listen on TCP server socket .\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    if (OSAL_SUCCESS != OSAL_netAcceptOnSocket(&socketFd, &connSocket,
            &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to accept TCP client.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Receive data from client. */ 
    size = sizeof(buf);
    OSAL_memSet(buf, 0, size);
    do {
        ret = OSAL_netSocketReceive(&connSocket, buf, &size,
                 OSAL_NET_RECV_NO_FLAGS);
    } while(!ret);

    /* Compare receiving msg with sending msg */
    if(0 != OSAL_memCmp(buf, net_ut_clientSendBuf,
            OSAL_strlen(net_ut_clientSendBuf))) {
        OSAL_logMsg("%s:%d Unexpected receive msg from client.\n", __FILE__, __LINE__);
    }
    OSAL_logMsg("TCP server recv msg=%s\n", buf);

    /* Close server socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("%s:%d Fails to close TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    net_ut_server_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_netTcpClient() ========
 *
 * Function to create TCP client socket, send data to server.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netTcpClient(
    OSAL_NetSockType  type)
{
    OSAL_NetSockId socketFd;
    int size;


    /* Create client socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("%s:%d Cannot create TCP client socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Connet to server. */
    if (OSAL_SUCCESS != OSAL_netConnectSocket(&socketFd, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Cannot connect to TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Send data to server. */
    OSAL_memCpy(net_ut_clientSendBuf, "TCP socket client send",
            OSAL_strlen("TCP socket client send"));
    size = OSAL_strlen(net_ut_clientSendBuf);    
    if (OSAL_SUCCESS != OSAL_netSocketSend(&socketFd,
            net_ut_clientSendBuf, &size)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to send data to TCP server.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Close client socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("%s:%d Fails to close TCP client socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    net_ut_client_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_netUdpServer() ========
 *
 * Function to create UDP server socket, receive data from client and send 
 * send data to client.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netUdpServer(
    OSAL_NetSockType  type)
{
    int            size;
    OSAL_NetSockId socketFd;
    char    buf[NET_UT_BUF_SIZE];
    OSAL_NetAddress  fromAddr;
    static int ret = OSAL_FAIL;
    
    /* Create Rx socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("Cannot create UDP server socket.\n");
        return (UT_FAIL);
    }

    /* Bind Rx socket. */  
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &net_ut_clientAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind UDP server socket.\n");
        return (UT_FAIL);
    }

    /* Receive data from client. */
    size = sizeof(buf);
    OSAL_memSet(buf, 0, size);
    fromAddr.type = type;
    do {         
        ret = OSAL_netSocketReceiveFrom(&socketFd,
            (void *)buf, &size, &fromAddr);
    } while(!ret);

    /* Compare receiving msg with sending msg. */
    if(0 != OSAL_memCmp(buf, net_ut_clientSendBuf,
            OSAL_strlen(net_ut_clientSendBuf))) {
        OSAL_logMsg("Unexpected receive msg from client.\n");
    }
    OSAL_logMsg("UDP server recv msg=%s\n", buf);

    /* Close server socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("Fails to close UDP server socket.\n");
        return (UT_FAIL);
    }

    net_ut_server_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_netUdpClient() ========
 *
 * Function to create UDP client socket, send data to server.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netUdpClient(
    OSAL_NetSockType  type)
{
    OSAL_NetSockId socketFd;
    int size;
    OSAL_NetAddress addr;


    /* Create Rx socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("Cannot create UDP client socket.\n");
        return (UT_FAIL);
    }
    /* 
     * Check if socket ID is valid.
     * Socket is be created and its ID should be valid.
     */
    if (OSAL_SUCCESS != OSAL_netIsSocketIdValid(&socketFd)) {
        OSAL_logMsg("Socket ID is invalid\n");
        return (UT_FAIL);
    }

    /* Bind Rx socket. */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot bind UDP client socket.\n");
        return (UT_FAIL);
    }

    /* send data to server */
    OSAL_memCpy(net_ut_clientSendBuf, "UDP socket client send",
            OSAL_strlen("UDP socket client send"));
    size = OSAL_strlen(net_ut_clientSendBuf);   

    if (OSAL_SUCCESS != OSAL_netSocketSendTo(&socketFd,
            net_ut_clientSendBuf, &size, &net_ut_clientAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Send failed.\n");
        return (UT_FAIL);
    }

    /* Test OSAL_netGetSocketAddress() by checking addr & port of server */
    if (OSAL_SUCCESS != OSAL_netGetSocketAddress(&socketFd, &addr)) {
        OSAL_logMsg("Fail to get socket addr.\n");
        return (UT_FAIL);
    }
    else {
        if (addr.port != net_ut_serverAddr.port) {
            OSAL_logMsg("The port get by OSAL_netGetSocketAddress() is wrong.\n");
            return (UT_FAIL);
        }
        if (addr.type == OSAL_NET_SOCK_UDP) {
            if (addr.ipv4 != net_ut_serverAddr.ipv4) {
                OSAL_logMsg("The ipv4 addr get by OSAL_netGetSocketAddress() is "
                        "wrong.\n");
                return (UT_FAIL);
            }
        }
        else if (addr.type == OSAL_NET_SOCK_UDP_V6) {
            if (OSAL_memCmp(addr.ipv6, net_ut_serverAddr.ipv6, 
                    OSAL_NET_IPV6_WORD_SZ)) {
                OSAL_logMsg("The ipv6 addr get by OSAL_netGetSocketAddress() is "
                        "wrong.\n");
                return (UT_FAIL);
            }
        }
    }

    /* Close client socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("Fails to close UDP client socket.\n");
        return (UT_FAIL);
    } 
     /* 
     * Check if socket ID is valid.
     * Socket is be closed and its ID should be invalid. 
     */
    if (OSAL_FAIL != OSAL_netIsSocketIdValid(&socketFd)) {
        OSAL_logMsg("Socket ID isn't expected as invalid.\n");
        return (UT_FAIL);
    }

    net_ut_client_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

static int _OSALUT_netTlscb(int preverify_ok, void *ctx)
{
    return (1);
}

/*
 * ======== _OSALUT_netTlsServer() ========
 *
 * Function to create TLS server socket, receive data from client and check
 * if the data is correct.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netTlsServer(
    OSAL_NetSockType  type)
{
    OSAL_NetSockId  socketFd;
    OSAL_NetSockId  connSocket;
    OSAL_NetSslCert cert;
    int             size;
    char            buf[NET_UT_BUF_SIZE];
    static int      ret = OSAL_FAIL;

    
    /* Create server socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("%s:%d Cannot create TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    /* Bind server socket. */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Cannot bind TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_netListenOnSocket(&socketFd)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to listen on TCP server socket .\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS != OSAL_netAcceptOnSocket(&socketFd, &connSocket,
            &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to accept TCP client.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Gen & set certificate.*/
    OSAL_memSet(&net_ut_sslServer, 0, sizeof(OSAL_NetSslId));
    OSAL_memSet(&cert, 0, sizeof(OSAL_NetSslCert));
    if (OSAL_SUCCESS !=
            OSAL_netSslCertGen(&cert,
            1024, /* Bit */
            0,    /* Serial number */
            365)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to gen cert.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=
            OSAL_netSslSetCertVerifyCB(&net_ut_sslServer, _OSALUT_netTlscb)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to set cert CB.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=
            OSAL_netSslSetCert(&net_ut_sslServer, &cert)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to set cert\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* SSL initialization.*/
    if (OSAL_SUCCESS !=
            OSAL_netSsl(&net_ut_sslServer, OSAL_NET_SSL_METHOD_SERVER_SSLV3)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to create SSL.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    /* 
     * Note: use the new socket id, "connSocket", wich is after TCP accept on.
     */
    if (OSAL_SUCCESS !=
            OSAL_netSslSetSocket(&connSocket, &net_ut_sslServer)) {
        OSAL_netSslClose(&net_ut_sslServer);
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to set SSL.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    /* Connect to the remote party. */
    if (OSAL_SUCCESS != OSAL_netSslAccept(&net_ut_sslServer)) {
         OSAL_netSslClose(&net_ut_sslServer);
         OSAL_netCloseSocket(&socketFd);
         OSAL_logMsg("%s:%d Failed to accept SSL connect.\n", __FILE__, __LINE__);
         return (-1);
    }

    /* Receive data from client. */ 
    size = sizeof(buf);
    OSAL_memSet(buf, 0, size);
    do {
        ret = OSAL_netSslReceive(&net_ut_sslServer, buf, &size);
    } while(!ret);
    /* Compare receiving msg with sending msg */
    if(0 != OSAL_memCmp(buf, net_ut_clientSendBuf, 
            OSAL_strlen(net_ut_clientSendBuf))) {
        OSAL_logMsg("%s:%d Unexpected receive msg from client.\n", __FILE__, __LINE__);
    }
    OSAL_logMsg("%s:%d TLS server recv msg=%s\n",  __FILE__, __LINE__, buf);

    /* Send data to client. */
    OSAL_memCpy(net_ut_serverSendBuf, "SSL server send",
            OSAL_strlen("SSL server send"));
    size = OSAL_strlen(net_ut_serverSendBuf);
    if (OSAL_SUCCESS != OSAL_netSslSend(&net_ut_sslServer,
            net_ut_serverSendBuf, &size)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to send data to SSL server.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }


    /* Close SSL connection */
    OSAL_netSslClose(&net_ut_sslServer);
    /* Close server socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("%s:%d Fails to close TCP server socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }


    net_ut_server_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_netTlsClient() ========
 *
 * Function to create TLS client socket, send data to server. 
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_netTlsClient(
    OSAL_NetSockType  type)
{
    OSAL_NetSockId socketFd;
    int size;
    char    buf[NET_UT_BUF_SIZE];
    static int ret = OSAL_FAIL;

    /* Create client socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd, type)) {
        OSAL_logMsg("Cannot create TCP client socket.\n");
        return (UT_FAIL);
    }
    /* Connet to server. */
    if (OSAL_SUCCESS != OSAL_netConnectSocket(&socketFd, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("Cannot connect to TCP server socket.\n");
        return (UT_FAIL);
    }

    /* SSL initialization.*/
    OSAL_memSet(&net_ut_sslClient, 0, sizeof(OSAL_NetSslId));
    if (OSAL_SUCCESS != 
            OSAL_netSsl(&net_ut_sslClient, OSAL_NET_SSL_METHOD_CLIENT_SSLV3)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to create SSL.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    if (OSAL_SUCCESS !=
            OSAL_netSslSetSocket(&socketFd, &net_ut_sslClient)) {
        OSAL_netSslClose(&net_ut_sslClient);
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to set SSL.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    /* Connect to the remote party. */
    if (OSAL_SUCCESS !=
            OSAL_netSslConnect(&net_ut_sslClient)) {
        OSAL_netSslClose(&net_ut_sslClient);
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Failed to connect SSL server.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    OSAL_logMsg("%s:%d OK to connect SSL server.\n", __FILE__, __LINE__);

    /* Send data to server. */
    OSAL_memCpy(net_ut_clientSendBuf, "SSL client send", 
            OSAL_strlen("SSL client send"));
    size = OSAL_strlen(net_ut_clientSendBuf);
    if (OSAL_SUCCESS != OSAL_netSslSend(&net_ut_sslClient,
            net_ut_clientSendBuf, &size)) {
        OSAL_netCloseSocket(&socketFd);
        OSAL_logMsg("%s:%d Fails to send data to SSL server.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }

    /* Receive data from server. */ 
    size = sizeof(buf);
    OSAL_memSet(buf, 0, size);
    do {
        ret = OSAL_netSslReceive(&net_ut_sslClient, buf, &size);
    } while(!ret); 
    /* Compare receiving msg with sending msg */
    if(0 != OSAL_memCmp(buf, net_ut_serverSendBuf,
            OSAL_strlen(net_ut_serverSendBuf))) {
        OSAL_logMsg("%s:%d Unexpected receive msg from client.\n", __FILE__, __LINE__);
    }
    OSAL_logMsg("%s:%d TLS server recv msg=%s\n",  __FILE__, __LINE__, buf);


    /* Close SSL connection */
    OSAL_netSslClose(&net_ut_sslClient);
    /* Close client socket. */
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd)) {
        OSAL_logMsg("%s:%d Fails to close TCP client socket.\n", __FILE__, __LINE__);
        return (UT_FAIL);
    }
    net_ut_client_completed = OSAL_SUCCESS;
    return (UT_PASS);
}

/*
 * ======== _OSALUT_tranceivTest() ========
 *
 * Function to create server and client task for data transmission.
 *
 * Return:
 *  UT_PASS: Exit normally.
 *  UT_FAIL: Invalid arguments.
 */
static UT_Return _OSALUT_tranceivTest(
    OSAL_TaskPtr   recvFx_ptr, 
    OSAL_TaskPtr   sendFx_ptr, 
    OSAL_TaskArg   arg)
{
    OSAL_TaskId tServerId, tClientId;  

    net_ut_server_completed = OSAL_FAIL;
    net_ut_client_completed = OSAL_FAIL;

    /* Create a server task. */
    if (recvFx_ptr != NULL) {
        tServerId = OSAL_taskCreate("netUtServer", OSAL_TASK_PRIO_NIC,
                (uint32) 8000, (OSAL_TaskPtr)recvFx_ptr, arg);
        if ((OSAL_TaskId) NULL == tServerId) {
            OSAL_logMsg("Unexpected OSAL_taskCreate() server task faulure - "
                    "expected non-NULL \n");
            return (UT_FAIL);
        }
        /* Wait for server socket ready. */
        OSAL_taskDelay(1000);
    }

    /* Create a client task. */
    if (sendFx_ptr != NULL) {
        tClientId = OSAL_taskCreate("netUtClient", OSAL_TASK_PRIO_NIC,
                (uint32) 8000, (OSAL_TaskPtr)sendFx_ptr, arg);
        if ((OSAL_TaskId) NULL == tClientId) {
            OSAL_logMsg("Unexpected OSAL_taskCreate() client task faulure - "
                    "expected non-NULL \n");
            return (UT_FAIL);
        }
        /* Wait for msg transmission completely. */
        OSAL_taskDelay(5000);
    }

    if ((net_ut_server_completed  != OSAL_SUCCESS) || 
            (net_ut_client_completed  != OSAL_SUCCESS)){
        return (UT_FAIL);
    }

    return (UT_PASS);
}

/*
 * ========= do_test_net() ========
 * Gen unit test vectors for each OSAL net function
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
UT_Return do_test_net(
    void)
{
    char ipv4LoAddrStr[OSAL_NET_IPV4_STR_MAX] = "127.0.0.1";
    char ipv6LoAddrStr[OSAL_NET_IPV6_STR_MAX] = "0:0:0:0:0:0:0:1";
    OSAL_NetSockId socketFd1, socketFd2;
    OSAL_NetAddress addr;
    int isReuse;
#ifdef OSAL_UT_NET_TOS
    int tosVal;
#endif
    int optVal = 0;

    OSAL_logMsg("Net Unit Test Starting...\n");
    /* Reset this before every test. */
    osal_utErrorsFound = 0;

    /* Init address */
    OSAL_netStringToAddress((int8 *)ipv4LoAddrStr, &net_ut_serverAddr);
    OSAL_netStringToAddress((int8 *)ipv6LoAddrStr, &net_ut_serverAddr);
    net_ut_serverAddr.port = OSAL_netHtons(NET_UT_SERVER_PORT);

    OSAL_netStringToAddress((int8 *)ipv4LoAddrStr, &net_ut_clientAddr);
    OSAL_netStringToAddress((int8 *)ipv6LoAddrStr, &net_ut_clientAddr);
    net_ut_clientAddr.port = OSAL_netHtons(NET_UT_CLIENT_PORT);

    /*
     * Net test 1:
     * Test IPV4 UDP socket.
     */
    OSAL_logMsg("Net test 1\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_UDP;
    net_ut_clientAddr.type = OSAL_NET_SOCK_UDP;
    if (UT_PASS != _OSALUT_tranceivTest((OSAL_TaskPtr)_OSALUT_netUdpServer, 
            (OSAL_TaskPtr)_OSALUT_netUdpClient, (void *) OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("UDP socket test failure. Error count:%d\n", osal_utErrorsFound);
    }

    /*
     * Net test 2:
     * Test IPV6 UDP socket.
     */
    OSAL_logMsg("Net test 2\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_UDP_V6;
    net_ut_clientAddr.type = OSAL_NET_SOCK_UDP_V6;
    if (UT_PASS != _OSALUT_tranceivTest((OSAL_TaskPtr)_OSALUT_netUdpServer, 
            (OSAL_TaskPtr)_OSALUT_netUdpClient, (void *) OSAL_NET_SOCK_UDP_V6)) {
        osal_utErrorsFound++;
        OSAL_logMsg("UDP_V6 socket test failure. Error count:%d \n", osal_utErrorsFound);
    }

    /*
     * Net test 3:
     * Test IPV4 TCP socket.
     */
    OSAL_logMsg("Net test 3\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_TCP;
    net_ut_clientAddr.type = OSAL_NET_SOCK_TCP;  
    if (UT_PASS != _OSALUT_tranceivTest((OSAL_TaskPtr)_OSALUT_netTcpServer, 
            (OSAL_TaskPtr)_OSALUT_netTcpClient, (void *) OSAL_NET_SOCK_TCP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("TCP socket test failure. Error count:%d \n",
                osal_utErrorsFound);
    }

    /*
     * Net test 4:
     * Test IPV6 UDP socket.
     */
    OSAL_logMsg("Net test 4\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_TCP_V6;
    net_ut_clientAddr.type = OSAL_NET_SOCK_TCP_V6;
    if (UT_PASS != _OSALUT_tranceivTest((OSAL_TaskPtr)_OSALUT_netTcpServer, 
            (OSAL_TaskPtr)_OSALUT_netTcpClient, (void *) OSAL_NET_SOCK_TCP_V6)) {
        osal_utErrorsFound++;
        OSAL_logMsg("TCP_V6 socket test failure. Error count:%d \n",
                osal_utErrorsFound);
    }

    /*
     * Net test 5:
     * Test OSAL_netSetSocketOptions() & OSAL_netSetSocketOptions()
     * with OSAL_NET_SOCK_REUSE by reusing of local address.
     */
    OSAL_logMsg("Net test 5\n");
    isReuse = 1;
    /* Create the first socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd1, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot create the first socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    /* Set the socket  OSAL_NET_SOCK_REUSE. */
    if (OSAL_SUCCESS != OSAL_netSetSocketOptions(&socketFd1, 
            OSAL_NET_SOCK_REUSE, isReuse)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot set the first socket option. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    /* Configure the addr. */
    addr.type = OSAL_NET_SOCK_UDP;
    OSAL_netStringToAddress((int8 *)ipv4LoAddrStr, &addr);
    addr.port = OSAL_netHtons(NET_UT_SERVER_PORT);
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd1, &addr)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot bind the first socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }

   /* Create the second socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd2, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot create the second socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    /* Set the socket  OSAL_NET_SOCK_REUSE. */
    if (OSAL_SUCCESS != OSAL_netSetSocketOptions(&socketFd2, 
            OSAL_NET_SOCK_REUSE, isReuse)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot set the second socket option. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    /* Bind socket to the same addr. */
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd2, &addr)) {
        OSAL_netCloseSocket(&socketFd2);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot bind the second socket to the same addr. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    /* Get socket option with OSAL_NET_SOCK_REUSE to check the value. */
    if (OSAL_SUCCESS != OSAL_netGetSocketOptions(&socketFd2, 
            OSAL_NET_SOCK_REUSE, &optVal)) {
        OSAL_netCloseSocket(&socketFd2);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot get socket option with OSAL_NET_SOCK_REUSE. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    if (optVal != isReuse) {
        osal_utErrorsFound++;
        OSAL_logMsg("The optVal get by OSAL_netGetSocketOptions() with "
                "OSAL_NET_SOCK_REUSE is wrong. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }

    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd1)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Fails to close the first socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd2)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Fails to close the second socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_6;
    }

_OSAL_UT_NET_TEST_6:
#ifdef OSAL_UT_NET_TOS
    /*
     * Net test 6:
     * Test OSAL_netSetSocketOptions() with OSAL_NET_IP_TOS by 
     * OSAL_netGetSocketOptions() to check the TOS value.
     */
    OSAL_logMsg("Net test 6\n");
    /* Create the first socket. */
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd1, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot create the first socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_7;
    }
    if (OSAL_SUCCESS != OSAL_netGetSocketOptions(&socketFd1, 
            OSAL_NET_IP_TOS, &tosVal)) {
        OSAL_netCloseSocket(&socketFd2);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot get socket option with OSAL_NET_IP_TOS. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_7;
    }

    /* Set the socket option with OSAL_NET_IP_TOS. */
    tosVal += 1;
    if (OSAL_SUCCESS != OSAL_netSetSocketOptions(&socketFd1, 
            OSAL_NET_IP_TOS, tosVal)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot set socket option with OSAL_NET_IP_TOS. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_7;
    }
    if (OSAL_SUCCESS != OSAL_netGetSocketOptions(&socketFd1, 
            OSAL_NET_IP_TOS, &optVal)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot get socket option with OSAL_NET_IP_TOS. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_7;
    }
    if (optVal != tosVal) {
        osal_utErrorsFound++;
        OSAL_logMsg("The optVal get by OSAL_netGetSocketOptions() with "
                "OSAL_NET_IP_TOSis wrong. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_7;
    }
_OSAL_UT_NET_TEST_7:
#endif
    /*
     * Net test 7: (Fail case)
     * Create a socket with NULL socket pointer.
     */
    OSAL_logMsg("Net test 7\n");
    if (OSAL_FAIL != OSAL_netSocket(NULL, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Create a socket with NULL socket ptr success isn't "
                "expected. Error count:%d\n",
                osal_utErrorsFound);
    }

    /*
     * Net test 8: (Fail case)
     * Close socket with wrong socket id or NULL socket pointer.
     */
    OSAL_logMsg("Net test 8\n");
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd1, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot to create socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_9;
    }
    /* Close socket with wrong socket id. */
    socketFd2 = -1;
    if (OSAL_FAIL != OSAL_netCloseSocket(&socketFd2)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Close the socket with wrong socket id success isn't "
                "expected. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_9;
    }
    /* Close socket with NULL socket pointer. */ 
    if (OSAL_FAIL != OSAL_netCloseSocket(NULL)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Close a socket with NULL socket ptr success isn't "
                "expected. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_9;
    }
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd1)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Fails to close the first socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_9;
    }

_OSAL_UT_NET_TEST_9:
    /*
     * Net test 9: (Fail case)
     * Bind socket to a wrong addr and NULL pointer.
     */
    OSAL_logMsg("Net test 9\n");
    if (OSAL_SUCCESS != OSAL_netSocket(&socketFd1, OSAL_NET_SOCK_UDP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot to create socket. Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }
    /* Configure a wrong addr. */
    addr.type = OSAL_NET_SOCK_UDP;
    OSAL_netStringToAddress("172.16.0.165", &addr);
    addr.port = OSAL_netHtons(NET_UT_SERVER_PORT);
    if (OSAL_FAIL != OSAL_netBindSocket(&socketFd1, &addr)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Bind socket to a wrong addr success isn't expected. "
                "Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }
    /* Bind socket with NULL pointer. */
    if (OSAL_FAIL != OSAL_netBindSocket(NULL, &addr)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Bind socket with NULL ptr success isn't expected. "
                "Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }
    if (OSAL_FAIL != OSAL_netBindSocket(&socketFd1, NULL)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Bind socket with NULL ptr success isn't expected. "
                "Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }
    /* Configure a correct addr. */
    net_ut_serverAddr.type = OSAL_NET_SOCK_UDP;
    if (OSAL_SUCCESS != OSAL_netBindSocket(&socketFd1, &net_ut_serverAddr)) {
        OSAL_netCloseSocket(&socketFd1);
        osal_utErrorsFound++;
        OSAL_logMsg("Cannot bind the first socket. "
                "Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }   
    if (OSAL_SUCCESS != OSAL_netCloseSocket(&socketFd1)) {
        osal_utErrorsFound++;
        OSAL_logMsg("Fails to close the first socket. "
                "Error count:%d\n",
                osal_utErrorsFound);
        goto _OSAL_UT_NET_TEST_END;
    }

_OSAL_UT_NET_TEST_END:
    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Net Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Net Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}

/*
 * ========= do_test_ssl() ========
 * Gen unit test vectors for each OSAL ssl function
 *
 * Return:
 *  UT_PASS: Exit normally.
 */
UT_Return do_test_ssl(
    void)
{
    char ipv4LoAddrStr[OSAL_NET_IPV4_STR_MAX] = "127.0.0.1";
    char ipv6LoAddrStr[OSAL_NET_IPV6_STR_MAX] = "0:0:0:0:0:0:0:1";

    OSAL_logMsg("%s:%d SSL Unit Test Starting...\n", __FILE__, __LINE__);
    /* Reset this before every test. */
    osal_utErrorsFound = 0;

    /* Init address */
    OSAL_netStringToAddress(ipv4LoAddrStr, &net_ut_serverAddr);
    OSAL_netStringToAddress(ipv6LoAddrStr, &net_ut_serverAddr);
    net_ut_serverAddr.port = OSAL_netHtons(NET_UT_SERVER_PORT);
    OSAL_netStringToAddress(ipv4LoAddrStr, &net_ut_clientAddr);
    OSAL_netStringToAddress(ipv6LoAddrStr, &net_ut_clientAddr);
    net_ut_clientAddr.port = OSAL_netHtons(NET_UT_CLIENT_PORT);


    /* Initializes SSL module. */
    OSAL_netSslInit();
        
    /*
     * Net test 1:
     * Test IPV4 SSL.
     */
    OSAL_logMsg("\nSSL test 1\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_TCP;
    net_ut_clientAddr.type = OSAL_NET_SOCK_TCP;  
    if (UT_PASS != _OSALUT_tranceivTest((void *)_OSALUT_netTlsServer, 
            (void *)_OSALUT_netTlsClient, (void *) OSAL_NET_SOCK_TCP)) {
        osal_utErrorsFound++;
        OSAL_logMsg("%s:%d SSL test failure. Error count:%d\n", __FILE__, __LINE__, osal_utErrorsFound);
    }

    /*
     * Net test 2:
     * Test IPV6 SSL.
     */
    OSAL_logMsg("\nNet test 2\n");
    net_ut_serverAddr.type = OSAL_NET_SOCK_TCP_V6;
    net_ut_clientAddr.type = OSAL_NET_SOCK_TCP_V6;
    if (UT_PASS != _OSALUT_tranceivTest((void *)_OSALUT_netTlsServer, 
            (void *)_OSALUT_netTlsClient, (void *) OSAL_NET_SOCK_TCP_V6)) {
        osal_utErrorsFound++;
        OSAL_logMsg("%s:%d SSL V6 test failure. Error count:%d\n",  __FILE__, __LINE__, osal_utErrorsFound);
    }


    if (0 == osal_utErrorsFound) {
        OSAL_logMsg("Net Unit Test Completed Successfully.\n");
        return (UT_PASS);
    }
    else {
        OSAL_logMsg("Net Unit Test Completed with %d errors.\n",
                osal_utErrorsFound);
        return (UT_FAIL);
    }
}

