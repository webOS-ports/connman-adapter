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
#include <networkmanager.h>
#include <networktechnology.h>
#include <networkservice.h>

#include "connmanagent.h"
#include "connectionsettings.h"

class WifiNetworkService : public QObject
{
    Q_OBJECT

public:
    WifiNetworkService(QObject *parent = 0);
    virtual ~WifiNetworkService();

    void start(LSPalmService *service);

    void provideInputForConnman(const QVariantMap& fields, const QDBusMessage& message);

    static bool cbGetStatus(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbSetState(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbFindNetworks(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbConnect(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetProfile(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetInfo(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbDeleteProfile(LSHandle* lshandle, LSMessage *message, void *user_data);
    static bool cbGetProfileList(LSHandle* lshandle, LSMessage *message, void *user_data);

    bool processGetStatusMethod(LSHandle *handle, LSMessage *message);
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
    enum ServiceState {
        IDLE,
        ASSOCIATION,
        CONFIGURATION,
        READY,
        ONLINE,
        DISCONNECT,
        FAILURE,
    };

    bool _wifiServiceActive;
    NetworkManager *_manager;
    NetworkTechnology *_wifiTechnology;
    LSHandle *_privateService;
    NetworkService *_currentService;
    ServiceState _stateOfCurrentService;
    ConnmanAgent _agent;
    ConnectionSettings _connectionSettings;

    bool checkForConnmanService(json_object *response);
    bool setWifiPowered(const bool &powered);
    bool isWifiPowered() const;
    QList<NetworkService*> listNetworks() const;
    bool connectWithSsid(const QString& ssid, json_object *request, json_object *response);
    bool connectWithProfileId(int id);

    ServiceState mapConnmanServiceStateToSingle(QString state);
    QString mapConnmanServiceStateToPalm(ServiceState state, ServiceState lastState);

    void sendConnectionStatusToSubscribers(const QString& state);

private slots:
    void updateTechnologies(const QMap<QString, NetworkTechnology*> &added,
                            const QStringList &removed);
    void managerAvailabilityChanged(bool available);

    void wifiPoweredChanged(bool powered);
    void currentServiceStateChanged(const QString& changedState);

private:
    Q_DISABLE_COPY(WifiNetworkService);
};

#endif
