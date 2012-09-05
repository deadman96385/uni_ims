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
#include "pv_2way_source.h"
#ifdef PV2WAY_WIN32_GUI
#include "testcaseparser.h"
#endif

#include "pv_2way_media.h"
#include "pv_mime_string_utils.h"

int PV2WaySource::AddCodec(PvmiMIOFileInputSettings& aFileSettings)
{
    int error = 0;
    if (aFileSettings.iMediaFormat == PVMF_MIME_FORMAT_UNKNOWN)
    {
        aFileSettings.iMediaFormat = PV2WayMedia::GetMediaFormat(aFileSettings.iFileName.get_cstr());
    }
#ifdef PV2WAY_WIN32_GUI
    TestCaseParser parser;
    // for sources parse the con file - want to find out the format
    if (!parser.ParseConFile(aFileSettings))
        return -1;
#endif
    OSCL_TRY(error, AddFormat(aFileSettings));
    return error;
}

int PV2WaySource::AddCodec(PVMFFileInputSettings& aFileSettings)
{
    int error = 0;
    OSCL_TRY(error, AddFormat(aFileSettings));
    return error;
}

int PV2WaySource::AddCodec(DummyMIOSettings& aSettings)
{
    int error = 0;
    OSCL_TRY(error, AddFormat(aSettings));
    return error;
}
