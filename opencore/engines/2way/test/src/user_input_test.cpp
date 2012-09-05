/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#include "user_input_test.h"

#define DTMF_TEST_INPUT '1'
#define DTMF_TEST_UPDATE false
#define DTMF_TEST_DURATION 20

#define ALPHANUMERIC_STRING_LENGTH 16
uint8 alphanumericTestString[ALPHANUMERIC_STRING_LENGTH] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};


void user_input_test::DoCancel()
{
}

void user_input_test::H324MConfigCommandCompletedL(PVMFCmdResp& aResponse)
{
    OSCL_UNUSED_ARG(aResponse);
}

void user_input_test::H324MConfigHandleInformationalEventL(PVMFAsyncEvent& aEvent)
{
    switch (aEvent.GetEventType())
    {
        case PV_INDICATION_USER_INPUT:
            break;
        case PV_INDICATION_USER_INPUT_CAPABILITY:
            break;
    }
}


void user_input_test::RstCmdCompleted()
{
    RunIfNotReady();
}

void user_input_test::DisCmdSucceeded()
{
    printf("Finished disconnecting \n");
    if (iH324MConfig)
        iH324MConfig->removeRef();
    iH324MConfig = NULL;
    reset();
}

void user_input_test::DisCmdFailed()
{
    printf("Finished disconnecting \n");
    if (iH324MConfig)
        iH324MConfig->removeRef();
    iH324MConfig = NULL;
    reset();
}

void user_input_test::ConnectSucceeeded()
{
    if (iH324MConfig == NULL)
    {
        test_is_true(false);
        disconnect();
    }
    H324MConfigInterface * t324Interface = (H324MConfigInterface *)iH324MConfig;
    t324Interface->SetObserver(this);
    iUserInputId = t324Interface->SendUserInput(iUserInput);
}

void user_input_test::InitFailed()
{
    test_is_true(false);
    RunIfNotReady();
}

bool user_input_test::start_async_test()
{
    if (iIsDTMF)
    {
        iUserInput = OSCL_NEW(CPVUserInputDtmf,
                              (DTMF_TEST_INPUT, DTMF_TEST_UPDATE, DTMF_TEST_DURATION));
    }
    else
    {
        iUserInput = OSCL_NEW(CPVUserInputAlphanumeric,
                              (alphanumericTestString, ALPHANUMERIC_STRING_LENGTH));
    }

    if (iUserInput == NULL)
    {
        test_is_true(false);
        return false;
    }


    return test_base::start_async_test();;
}





