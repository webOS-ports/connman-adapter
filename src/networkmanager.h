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

#ifndef CONNMAN_MANAGER_H_
#define CONNMAN_MANAGER_H_

#include <QtDBus>

#include <luna-service2/lunaservice.h>

#include "connmanmanager.h"

class NetworkManager : QObject
{
    Q_OBJECT
public:
    NetworkManager(QObject *parent = 0);
    virtual ~NetworkManager();

    void start(LSPalmService *service);

    static bool cbGetState(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbSetState(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbFindNetworks(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbConnect(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetProfile(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetInfo(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbDeleteProfile(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetProfileList(LSHandle* lshandle, LSMessage *message, void *user_data);

    bool processGetStateMethod(LSHandle *handle, LSMessage *message);
    bool processSetStateMethod(LSHandle *handle, LSMessage *message);
    bool processFindNetworksMethod(LSHandle *handle, LSMessage *message);
    bool processConnectMethod(LSHandle *handle, LSMessage *message);
    bool processGetProfileMethod(LSHandle *handle, LSMessage *message);
    bool processDeleteProfileMethod(LSHandle *handle, LSMessage *message);
    bool processGetProfileListMethod(LSHandle *handle, LSMessage *message);
    bool processGetInfoMethod(LSHandle *handle, LSMessage *message);

signals:
    void availabilityChanged(bool available);

private:
    ConnmanManager *_manager;
    QDBusServiceWatcher *_serviceWatcher;
    bool _serviceIsAvailable;
    QDBusPendingCallWatcher *_getPropertiesWatcher;
    QVariantMap _propertiesCache;

private slots:
    void connectToConnman(QString = "");
    void disconnectFromConnman(QString = "");
    void connmanUnregistered(QString = "");
    void getPropertiesReply(QDBusPendingCallWatcher *call);
    void propertyChanged(const QString &name, const QDBusVariant &value);

private:
    Q_DISABLE_COPY(NetworkManager);
};

#endif