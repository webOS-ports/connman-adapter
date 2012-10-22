/*
 * @@@LICENSE
 *
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * LICENSE@@@
 */

#include "servicemgr.h"

ServiceManager::ServiceManager()
{
}

ServiceManager::~ServiceManager()
{
}

bool ServiceManager::start(GMainLoop *mainloop)
{
    int ret;
    LSError lserror;

    LSErrorInit(&lserror);

    ret = LSRegisterPalmService("com.palm.wifi", &_publicService, &lserror);
    if (!ret) {
        g_critical("Fatal - Could not initialize connman-adapter.  Is LunaService Down?. %s", lserror.message);
        LSErrorFree(&lserror);
        return 0;
    }

    ret = LSGmainAttachPalmService(_publicService, mainloop, &lserror);
    if (!ret) {
        g_critical("Fatal - Could not initialize connman-adapter.  Is LunaService Down?. %s", lserror.message);
        LSErrorFree(&lserror);
        return 0;
    }

    _privateServiceHandle = LSPalmServiceGetPrivateConnection(_publicService);

    _wifiNetworkService.start(_publicService);
}

void ServiceManager::stop()
{
}
