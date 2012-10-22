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

#include <cjson/json.h>

#include "networkmanager.h"
#include "utilities.h"

static LSMethod _serviceMethods[]  = {
    { "getstatus", NetworkManager::cbGetStatus },
    { "setstate", NetworkManager::cbSetState },
    { "findnetworks", NetworkManager::cbFindNetworks },
    { "connect", NetworkManager::cbConnect },
    { "getprofile", NetworkManager::cbGetProfile },
    { "getinfo", NetworkManager::cbGetInfo },
    { "deleteprofile", NetworkManager::cbDeleteProfile },
    { "getprofilelist", NetworkManager::cbGetProfileList },
    { 0, 0 }
};

NetworkManager::NetworkManager(QObject *parent) :
    QObject(parent),
    _manager(NULL),
    _serviceWatcher(NULL),
    _serviceIsAvailable(false),
    _wifiServiceActive(false)
{
    registerCommonDataTypes();
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::start(LSPalmService *service)
{
    int ret;
    LSError lserror;

    LSErrorInit(&lserror);

    ret = LSPalmServiceRegisterCategory(service, "/",
                NULL, _serviceMethods, NULL, this, &lserror);
    if (!ret) {
        g_error("Failed to register service category /");
        return;
    }

    _serviceWatcher = new QDBusServiceWatcher("net.connman",QDBusConnection::systemBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, this);

    connect(_serviceWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(connectToConnman(QString)));
    connect(_serviceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(connmanUnregistered(QString)));

    _serviceIsAvailable = QDBusConnection::systemBus().interface()->isServiceRegistered("net.connman");

    if (_serviceIsAvailable)
        connectToConnman();
}

void NetworkManager::connectToConnman(QString)
{
    disconnectFromConnman();
    _manager = new ConnmanManager("net.connman", "/", QDBusConnection::systemBus(), this);

    if (!_manager->isValid()) {
        qDebug("manager is invalid. connman may not be running or is invalid");
        delete _manager;
        _manager = NULL;

        if(_serviceIsAvailable) {
            _serviceIsAvailable = false;
            // FIXME fire a luna service signal here
        }
    } else {
        _getPropertiesWatcher = new QDBusPendingCallWatcher(_manager->GetProperties(), _manager);
        connect(_getPropertiesWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(getPropertiesReply(QDBusPendingCallWatcher*)));

        if(!_serviceIsAvailable) {
            _serviceIsAvailable = true;
            // FIXME fire a luna service signal here
        }

        qDebug("connected");
    }
}

void NetworkManager::disconnectFromConnman(QString)
{
    if (_manager) {
        delete _manager;
        _manager = NULL;
    }
}

void NetworkManager::connmanUnregistered(QString)
{
    disconnectFromConnman();

    if(_serviceIsAvailable)
        emit availabilityChanged(_serviceIsAvailable = false);
}

void NetworkManager::getPropertiesReply(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    if (reply.isError()) {
        // qDebug() << "Error getPropertiesReply: " << reply.error().message();
        disconnectFromConnman();
        QTimer::singleShot(10000, this, SLOT(connectToConnman));
    }
    else {
        _propertiesCache = reply.value();
        connect(_manager, SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
                this, SLOT(propertyChanged(const QString&, const QDBusVariant&)));
    }
}

void NetworkManager::propertyChanged(const QString &name, const QDBusVariant &value)
{
    _propertiesCache[name] = value.variant();
    // FIXME fire a luna service signal here
}

bool NetworkManager::processGetStatusMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    json_object_object_add(response, "returnValue", json_object_new_boolean(true));
    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(response, "status",
        json_object_new_string(_wifiServiceActive ? "serviceEnabled" : "serviceDisabled"));

    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool NetworkManager::processSetStateMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

bool NetworkManager::processFindNetworksMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

bool NetworkManager::processConnectMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

bool NetworkManager::processGetProfileMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

bool NetworkManager::processGetInfoMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    json_object_object_add(response, "returnValue", json_object_new_boolean(true));

    /* default values until we have something real */
    json_object_object_add(response, "macAddress", json_object_new_string("ff:ff:ff:ff:ff:ff"));
    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(response, "wmm", json_object_new_string("disabled"));
    json_object_object_add(response, "roaming", json_object_new_string("disabled"));
    json_object_object_add(response, "powerSave", json_object_new_string("enabled"));

    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool NetworkManager::processDeleteProfileMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

bool NetworkManager::processGetProfileListMethod(LSHandle *handle, LSMessage *message)
{
    return true;
}

#define LS2_CB_METHOD(name) \
bool NetworkManager::cb##name(LSHandle* lshandle, LSMessage *message, void *user_data) \
{ \
    NetworkManager *self = (NetworkManager*) user_data; \
    LSMessageRef(message); \
    return self->process##name##Method(lshandle, message); \
}

LS2_CB_METHOD(GetStatus)
LS2_CB_METHOD(SetState)
LS2_CB_METHOD(FindNetworks)
LS2_CB_METHOD(Connect)
LS2_CB_METHOD(GetProfile)
LS2_CB_METHOD(GetInfo)
LS2_CB_METHOD(DeleteProfile)
LS2_CB_METHOD(GetProfileList)
