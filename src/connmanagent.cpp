/*
 * @@@LICENSE
 *
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
 *
 * Some of the code is fairly copied from libconnman-qt under the terms of the Apache 2.0
 * license and is
 * Copyright (c) 2010, Intel Corporation.
 * Copyright (c) 2012, Jolla.

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

#include "connmanagent.h"
#include "wifiservice.h"

ConnmanAgent::ConnmanAgent(WifiNetworkService* parent)
    : QDBusAbstractAdaptor(parent),
      _service(parent)
{
}

ConnmanAgent::~ConnmanAgent()
{
}

void ConnmanAgent::Release()
{
}

void ConnmanAgent::ReportError(const QDBusObjectPath &service_path, const QString &error)
{
    qDebug() << "From " << service_path.path() << " got this error:\n" << error;
}

void ConnmanAgent::RequestBrowser(const QDBusObjectPath &service_path, const QString &url)
{
    qDebug() << "Service " << service_path.path() << " wants browser to open hotspot's url " << url;
}

void ConnmanAgent::RequestInput(const QDBusObjectPath &service_path,
    const QVariantMap &fields, const QDBusMessage &message)
{
    QVariantMap json;
    qDebug() << "Service " << service_path.path() << " wants user input";

    _service->provideInputForConnman(fields, message);
}

void ConnmanAgent::Cancel()
{
    qDebug() << "WARNING: request to agent got canceled";
}
