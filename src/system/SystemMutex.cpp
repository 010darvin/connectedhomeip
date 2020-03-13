/*
 *
 *    <COPYRIGHT>
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file defines the abstraction of mutual exclusion locks
 *      offered by the target platform.
 */

// Include module header
#include <SystemMutex.h>

// Include common private header
#include "SystemLayerPrivate.h"

#if !CHIP_SYSTEM_CONFIG_NO_LOCKING

// Include system headers
#include <errno.h>

namespace chip {
namespace System {

/**
 * Initialize the mutual exclusion lock instance.
 *
 *  @param[inout]   aThis   A zero-initialized object.
 *
 *  @retval         #CHIP_SYSTEM_NO_ERROR                  The mutual exclusion lock is ready to use.
 *  @retval         #CHIP_SYSTEM_ERROR_NO_MEMORY           Insufficient system memory to allocate the mutual exclusion lock.
 *  @retval         #CHIP_SYSTEM_ERROR_UNEXPECTED_STATE    An unexpected system error encountered during initialization.
 */

#if CHIP_SYSTEM_CONFIG_POSIX_LOCKING
DLL_EXPORT Error Mutex::Init(Mutex & aThis)
{
    int lSysError = pthread_mutex_init(&aThis.mPOSIXMutex, NULL);
    Error lError;

    switch (lSysError)
    {
    case 0:
        lError = CHIP_SYSTEM_NO_ERROR;
        break;

    case ENOMEM:
        lError = CHIP_SYSTEM_ERROR_NO_MEMORY;
        break;

    default:
        lError = CHIP_SYSTEM_ERROR_UNEXPECTED_STATE;
        break;
    }

    return lError;
}
#endif // CHIP_SYSTEM_CONFIG_POSIX_LOCKING

#if CHIP_SYSTEM_CONFIG_FREERTOS_LOCKING
DLL_EXPORT Error Mutex::Init(Mutex & aThis)
{
restart:
    if (__sync_bool_compare_and_swap(&aThis.mInitialized, 0, 1))
    {
#if (configSUPPORT_STATIC_ALLOCATION == 1)
        aThis.mFreeRTOSSemaphore = xSemaphoreCreateMutexStatic(&aThis.mFreeRTOSSemaphoreObj);
#else
        aThis.mFreeRTOSSemaphore = xSemaphoreCreateMutex();
#endif
        if (aThis.mFreeRTOSSemaphore == NULL)
        {
            aThis.mInitialized = 0;

            return CHIP_SYSTEM_ERROR_NO_MEMORY;
        }
    }
    else
    {
        while (aThis.mFreeRTOSSemaphore == NULL)
        {
            vTaskDelay(1);

            if (aThis.mInitialized == 0)
            {
                goto restart;
            }
        }
    }

    return CHIP_SYSTEM_NO_ERROR;
}

DLL_EXPORT void Mutex::Lock(void)
{
    xSemaphoreTake(this->mFreeRTOSSemaphore, portMAX_DELAY);
}
#endif // CHIP_SYSTEM_CONFIG_FREERTOS_LOCKING

} // namespace System
} // namespace chip

#endif // !CHIP_SYSTEM_CONFIG_NO_LOCKING
