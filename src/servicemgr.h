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

#ifndef SERVICEMGR_H_
#define SERVICEMGR_H_

#include <glib.h>
#include <luna-service2/lunaservice.h>

#include "wifiservice.h"

class ServiceManager
{
public:
    ServiceManager();
    ~ServiceManager();

    bool start(GMainLoop *mainloop);
    void stop();

private:
    LSPalmService *_publicService;
    LSHandle *_privateServiceHandle;
    WifiNetworkService _wifiNetworkService;
};

#endif // SERVICEMGR_H_
