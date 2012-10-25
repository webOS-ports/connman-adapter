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

#ifndef USERINPUTAGENT_H_
#define USERINPUTAGENT_H_

#include <QtDBus>

class WifiNetworkService;

class ConnmanAgent : public QDBusAbstractAdaptor
{
    Q_OBJECT;
    Q_CLASSINFO("D-Bus Interface", "net.connman.Agent");

public:
    ConnmanAgent(WifiNetworkService *parent);
    virtual ~ConnmanAgent();

public slots:
    void Release();
    void ReportError(const QDBusObjectPath &service_path, const QString &error);
    void RequestBrowser(const QDBusObjectPath &service_path, const QString &url);
    Q_NOREPLY void RequestInput(const QDBusObjectPath &service_path,
                                const QVariantMap &fields,
                                const QDBusMessage &message);
    void Cancel();

private:
    WifiNetworkService *_service;
};


#endif
