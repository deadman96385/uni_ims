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

#include "connect_cancel_test.h"



void connect_cancel_test::DoCancel()
{
}

void connect_cancel_test::ConnectSucceeded()
{
    CancelTimers();
    disconnect();
}

void connect_cancel_test::InitSucceeded()
{
    int error;
    test_base::InitSucceeded();
    OSCL_TRY(error, iCancelCmdId = terminal->CancelAllCommands());
    if (error)
    {
        test_is_true(false);
        reset();
    }
}


void connect_cancel_test::ConnectCancelled()
{
}
void connect_cancel_test::DisCmdSucceeded()
{
    test_is_true(true);
    reset();
}

void connect_cancel_test::DisCmdFailed()
{
    test_is_true(true);
    reset();
}
void connect_cancel_test::CancelCmdCompleted()
{
    test_is_true(true);
    reset();
}




