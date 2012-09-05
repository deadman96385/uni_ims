/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
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

#include "init_cancel_test.h"

void init_cancel_test::test()
{
    PV2WayUtil::OutputInfo("----- Start %s test, proxy %d. ----- \n", iTestName.get_cstr(), iUseProxy);
    PV2WayUtil::OutputInfo("\n** Test Number: %d. ** \n", iTestNum);
    int error = 0;

    scheduler = OsclExecScheduler::Current();

    this->AddToScheduler();

    iStarttime = OsclTickCount::TicksToMsec(OsclTickCount::TickCount());
    if (start_async_test())
    {
        OSCL_TRY(error, scheduler->StartScheduler());
        if (error != 0)
        {
            OSCL_LEAVE(error);
        }
    }

    TestCompleted();
    this->RemoveFromScheduler();
}


void init_cancel_test::DoCancel()
{
}

void init_cancel_test::InitSucceeded()
{
    test_is_true(true);
    reset();
}

void init_cancel_test::InitCancelled()
{
    InitSucceeded();
}

void init_cancel_test::InitFailed()
{
    test_is_true(false);
    RunIfNotReady();
}


void init_cancel_test::QueryInterfaceSucceeded()
{
    test_base::QueryInterfaceSucceeded();

    int error = 0;
    OSCL_TRY(error, iCnclCmdId = terminal->CancelAllCommands());
    if (error)
    {
        test_is_true(false);
    }

}


