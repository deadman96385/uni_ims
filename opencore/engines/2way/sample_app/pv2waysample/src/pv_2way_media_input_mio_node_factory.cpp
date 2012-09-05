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

#include "pv_2way_media_input_mio_node_factory.h"
#include "pv_2way_interface.h"
#include "pv_2way_media.h"
#include "pvmf_media_input_node_factory.h"

int PV2WayMediaInputMIONodeFactory::CreateMedia(PvmiMIOFileInputSettings& aFileSettings)
{
    iMediaControl = PvmiMIOFileInputFactory::Create(aFileSettings);
    return PVMFSuccess;
}

void PV2WayMediaInputMIONodeFactory::DeleteMedia()
{
    PvmiMIOFileInputFactory::Delete(iMediaControl);
    iMediaControl = NULL;
}

OSCL_EXPORT_REF void PV2WayMediaInputMIONodeFactory::Delete(PVMFNodeInterface** aMioNode)
{
    PvmfMediaInputNodeFactory::Delete(*aMioNode);
    DeleteMedia();
    *aMioNode = NULL;
}

OSCL_EXPORT_REF PVMFNodeInterface* PV2WayMediaInputMIONodeFactory::Create(PvmiMIOFileInputSettings& aFileSettings)
{
    CreateMedia(aFileSettings);
    PVMFNodeInterface* mioNode = NULL;
    int error = 0;
    OSCL_TRY(error, mioNode = PvmfMediaInputNodeFactory::Create(iMediaControl));
    return mioNode;
}

void PV2WayMediaInputMIONodeFactory::Release()
{
    delete this;
}


