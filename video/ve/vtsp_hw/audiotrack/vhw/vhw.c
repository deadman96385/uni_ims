/*
 * THIS IS AN UNPUBLISHED WORK CONTAINING D2 TECHNOLOGIES, INC. CONFIDENTIAL
 * AND PROPRIETARY INFORMATION.  IF PUBLICATION OCCURS, THE FOLLOWING NOTICE
 * APPLIES: "COPYRIGHT 2005-2009 D2 TECHNOLOGIES, INC. ALL RIGHTS RESERVED"
 *
 * $D2Tech$ $Rev: 16048 $ $Date: 2011-11-15 08:16:21 +0800 (Tue, 15 Nov 2011) $
 */
#include "vhw.h"
#include "jvhw.h"
#include "_vhw.h"

/*
 * Allocate an object that contains parameters that are shared within this
 * module.
 */
static _VHW_Object  _VHW_object;
/*
 * Store the name of the queue used to command the JVHW module.
 */
static char        *_VHW_cmdIpc_ptr = JVHW_CMD_IPC;
/*
 * Store the name of the queue that contains data received from the D/A
 * converter.
 */
static char        *_VHW_rxIpc_ptr  = JVHW_RX_IPC;
/*
 * Store the name of the queue that is used to send data to the speaker.
 */
static char        *_VHW_txIpc_ptr  = JVHW_TX_IPC;

/*
 * This local variable contains the address of the buffer used to hold data
 * received from the microphone.
 */
static int         *rxBuffer_ptr;
/*
 * This local variable contains the address of the buffer used to hold data
 * that will be sent to the speakers.
 */
static int         *txBuffer_ptr;
/*
 * This flag is used to indicate the state of the JVHW driver. When isSleeping
 * is set to 1, that means JVHW is not communicating with the speaker and
 * microphone. When isSleeping is set to 0, JVHW is collecting data from the
 * microphone and sending data to the speaker.
 */
static int          isSleeping = 1;
/*
 * When JVHW is run in a separate process, this flag is used to indicate that
 * VHW is waiting for JVHW to restart. When the flag is set to 1, VHW has lost
 * communications with VHW. When set to 0, communications is normal between the
 * modules.
 */
static int          recoverFlag = 0;

/*
 * This buffer is temporary storage for audio frames. The Java audio routines
 * use shorts. VTSPR uses ints. This buffer holds the data until it is
 * converted.
 */
static short       *qBuffer_ptr;

/*
 * ======== recover ========
 * This function is called when communications is lost between the VHW and JVHW
 * modules. When JVHW restarts, it will look for commands from VHW. In the case
 * where voice is being streamed, JVHW will read these commands and resume
 * exchanging data with the application.
 *
 * Return value:
 * none.
 */
static void recover(
    void)
{
    if (recoverFlag == 0) {
        VHW_start();
        _VHW_attach();
        recoverFlag = 1;
    }
}

/*
 * ======== cleanup ========
 * This function is used to cleanup after the VHW and JVHW drivers have stopped.
 *
 * Return value:
 * none
 */
static void cleanup(
    void)
{
    /*
     * Destroy Queues.
     */
    if (_VHW_object.cmdQueue.cmdIpcId != NULL) {
        OSAL_msgQDelete(_VHW_object.cmdQueue.cmdIpcId);
    }
    if (_VHW_object.rxQueue.rxQueueId != NULL) {
        OSAL_msgQDelete(_VHW_object.rxQueue.rxQueueId);
    }
    if (_VHW_object.txQueue.txQueueId != NULL) {
        OSAL_msgQDelete(_VHW_object.txQueue.txQueueId);
    }
    /*
     * Free Buffers.
     */
    if (rxBuffer_ptr != NULL) {
        OSAL_memFree(rxBuffer_ptr, 0);
        rxBuffer_ptr = NULL;
    }
    if (txBuffer_ptr != NULL) {
        OSAL_memFree(txBuffer_ptr, 0);
        txBuffer_ptr = NULL;
    }

    if (qBuffer_ptr != 0) {
        OSAL_memFree(qBuffer_ptr, 0);
        qBuffer_ptr = NULL;
    }

    /*
     * Completely clear object.
     */
    OSAL_memSet(&_VHW_object, 0, sizeof(_VHW_Object));
}

/*
 * ======== VHW_init ========
 * Initialize this module. This function must be called before any other VHW
 * function. It allocates all static memory needed by the module and establishes
 * the queues that it uses to communicate with JVHW.
 *
 * Return value:
 * 0 upon success.
 * 1 indicates failure to initialize.
 */
int VHW_init(
    void)
{
    /* Cache the name for the JVHW command queue */
    OSAL_snprintf(_VHW_object.cmdQueue.cmdIpcName, _VHW_STRING_SZ, "%s",
        _VHW_cmdIpc_ptr);

    /* Create queue for JVHW commands. */
    if (0 == (_VHW_object.cmdQueue.cmdIpcId = OSAL_msgQCreate(_VHW_cmdIpc_ptr,
            JVHW_MAX_QUEUE_DEPTH, sizeof(JVHW_Command), 0))) {
        cleanup();
        return (1);
    }

    /* Cache the name of the queue used for getting data from the microphone */
    OSAL_snprintf(_VHW_object.rxQueue.rxIpcName, _VHW_STRING_SZ, "%s",
        _VHW_rxIpc_ptr);

    /* Create the queue used for receiving microphone data. */
    if (0 == (_VHW_object.rxQueue.rxQueueId = OSAL_msgQCreate(_VHW_rxIpc_ptr,
            JVHW_MAX_QUEUE_DEPTH, JVHW_PCM_BUFFER_SIZE, 0))) {
        cleanup();
        return (1);
    }

    /* Cache the name of the queue used for sending data to the speakers. */
    OSAL_snprintf(_VHW_object.txQueue.txIpcName, _VHW_STRING_SZ, "%s",
        _VHW_txIpc_ptr);

    /* Create the queue used for sending data to the speakers. */
    if (0 == (_VHW_object.txQueue.txQueueId = OSAL_msgQCreate(_VHW_txIpc_ptr,
            JVHW_MAX_QUEUE_DEPTH, JVHW_PCM_BUFFER_SIZE, 0))) {
        cleanup();
        return (1);
    }

    /* Allocate memory to hold microphone data received from JVHW. */
    if (NULL == (rxBuffer_ptr = (int *) OSAL_memCalloc(_VHW_PCM_BUFFER_SIZE,
            1, 0))) {
        cleanup();
        return (1);
    }

    /* Allocate memory used to hold data that will be sent to the speaker. */
    if (NULL == (txBuffer_ptr = (int *) OSAL_memCalloc(_VHW_PCM_BUFFER_SIZE,
            1, 0))) {
        cleanup();
        return (1);
    }

    if (NULL == (qBuffer_ptr = (short *) OSAL_memCalloc(JVHW_PCM_BUFFER_SIZE,
            1, 0))) {
        cleanup();
        return(1);
    }
    isSleeping = 1;
    recoverFlag = 0;
    return (0);
}

/*
 * ======== VHW_start ========
 * Start the driver. In this case, VHW start will issue a command to start
 * the JVHW module.
 *
 * Return value:
 * none
 */
void VHW_start(
    void)
{
    JVHW_Command command;

    OSAL_logMsg("VHW Starting\n");
    isSleeping = 1;
    command = JVHW_CMD_START;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.cmdQueue.cmdIpcId, &command,
            sizeof(JVHW_Command), _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("VHW: Failed to Send START message\n");
    }
}

/*
 * ======== VHW_shutdown ========
 * This command is use to end all operations of the VHW module. After is signals
 * JVHW to shutdown. It destoys its queues and deallocates memory.
 *
 * Return value:
 * none
 */
void VHW_shutdown(
    void)
{
    JVHW_Command command;

    OSAL_logMsg("VHW Shutdown\n");
    isSleeping = 1;
    command = JVHW_CMD_DESTROY;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.cmdQueue.cmdIpcId, &command,
            sizeof(JVHW_Command), _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("VHW: Failed to Send DESTROY message\n");
    }
    /* Destroy queues and deallocate memory */
    cleanup();
}

/*
 * ======== VHW_exchange ========
 * Exchange data with the microphone (rx) and the speaker (tx). When called, tx
 * data is immediately sent. This function will block on receiving rx data from
 * the driver.
 *
 * Return Values:
 *
 * *rx_ptr contains the address of the rx data.
 * *tx_ptr contains the address of the tx data.
 */
void VHW_exchange(
    int **tx_ptr,
    int **rx_ptr)
{
    int index;
    *tx_ptr = txBuffer_ptr;
    *rx_ptr = rxBuffer_ptr;
    /*
     * Place transmit data into queue.
     */
    for (index = 0; index < _VHW_PCM_BUF_SZ; index++) {
        qBuffer_ptr[index] = (short) txBuffer_ptr[index];
    }
    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.txQueue.txQueueId, qBuffer_ptr,
            JVHW_PCM_BUFFER_SIZE, _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("Unable to to send tx data!\n");
    }
    /*
     * Wait for rx data.
     */
    if (OSAL_msgQRecv(_VHW_object.rxQueue.rxQueueId, qBuffer_ptr,
            JVHW_PCM_BUFFER_SIZE, _VHW_JVHW_TIMEOUT_MS, NULL) <= 0) {
        /*
         * If data does not arrive in a timely manner, try to recover. After
         * recovery is initiated, set the tx and rx buffers to 0 and allow the
         * application to proceed.
         */
        if (recoverFlag == 0) {
            OSAL_logMsg("VHW: Failed to receive rx data\n");
            recover();
        }
        OSAL_memSet(rxBuffer_ptr, 0, _VHW_PCM_BUFFER_SIZE);
        OSAL_memSet(txBuffer_ptr, 0, _VHW_PCM_BUFFER_SIZE);
    }
    else {
        // TODO convert short to int.
        for (index = 0; index < _VHW_PCM_BUF_SZ; index++) {
            rxBuffer_ptr[index] = (int) qBuffer_ptr[index];
        }
        recoverFlag = 0;
    }
}

/*
 * ======== _VHW_attach ========
 * Enable the flow of rx (microphone) and tx (speaker) data between this module
 * and the JVHW module.
 */
void _VHW_attach(
    void)
{

    JVHW_Command command;

    OSAL_logMsg("VHW Attach\n");
    /*
     * Flush queues before transitioning out of sleeping.
     */
    if (isSleeping != 0) {
        while ( OSAL_msgQRecv(_VHW_object.txQueue.txQueueId, qBuffer_ptr,
            JVHW_PCM_BUFFER_SIZE, OSAL_NO_WAIT, NULL) > 0);
        while ( OSAL_msgQRecv(_VHW_object.rxQueue.rxQueueId, qBuffer_ptr,
            JVHW_PCM_BUFFER_SIZE, OSAL_NO_WAIT, NULL) > 0);
    }
    isSleeping = 0;
    command = JVHW_CMD_ATTACH;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.cmdQueue.cmdIpcId, &command,
                    sizeof(JVHW_Command), _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("VHW: Failed to Send ATTACH message\n");
    }
}

/*
 * ======== _JVHW_detach ========
 * Disable the flow of data between this module and JVHW. This allows JVHW to go
 * to a lower power state where is is not running.
 */
void _VHW_detach(
    void)
{
    JVHW_Command command;

    OSAL_logMsg("VHW Detach\n");
    /*
     * Set the isSleeping flag.
     */
    isSleeping = 1;
    command = JVHW_CMD_DETACH;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.cmdQueue.cmdIpcId, &command,
            sizeof(JVHW_Command), _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("VHW: Failed to Send DETACH message\n");
    }
    recoverFlag = 0;
}

/*
 * ======== VHW_stop ========
 * Signal the JVHW driver. The driver can resume operations with a start
 * command.
 */
void VHW_stop(
    void)
{
    JVHW_Command command;

    OSAL_logMsg("VHW Stop\n");
    isSleeping = 1;
    command = JVHW_CMD_STOP;

    if (OSAL_SUCCESS != OSAL_msgQSend(_VHW_object.cmdQueue.cmdIpcId, &command,
            sizeof(JVHW_Command), _VHW_JVHW_TIMEOUT_MS, NULL)) {
        OSAL_logMsg("VHW: Failed to Send STOP message\n");
    }
}

/*
 * ======== VHW_isSleeping ========
 * Query this module as to the state of the audio driver.
 *
 * Return Value:
 * 0 Audio driver is active.
 * 1 Audio driver is inactive. VHW_exchange will block till timeout.
 */
int VHW_isSleeping(
    void)
{
    return (isSleeping);
}

