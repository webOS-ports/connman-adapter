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

static LSMethod _connmanManagerMethods[]  = {
    /* NOTE: internal method to check wether the connman dbus service is running */
    { "checkAvailable", NetworkManager::cbCheckAvailable },

    { "getProperties", NetworkManager::cbGetProperties },
    { "setProperty", NetworkManager::cbSetProperty },

    { "getTechnologies", NetworkManager::cbGetTechnologies },
    { "getServices", NetworkManager::cbGetServices },

    /* NOTE: the missing methods of the net.connman.Manager API will not be implemented
     * int the first version but should be addded later. */

    { 0, 0 }
};

NetworkManager::NetworkManager(QObject *parent) :
    QObject(parent),
    _manager(NULL),
    _serviceWatcher(NULL),
    _serviceIsAvailable(false)
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

    ret = LSPalmServiceRegisterCategory(service, "/manager",
                NULL, _connmanManagerMethods, NULL, this, &lserror);
    if (!ret) {
        g_error("Failed to register service category /manager");
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

bool NetworkManager::processGetPropertiesMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    QMap<QString,QVariant>::iterator iter;
    LSError lsError;

    LSErrorInit(&lsError);

    /* We have all properties already cached so we can just push them out here */
    response = json_object_new_object();

    for (iter = _propertiesCache.begin(); iter != _propertiesCache.end(); ++iter) {
        json_object_object_add(response,
                               iter.key().toUtf8().data(),
                               convertQVariantToJsonObject(iter.value()));
    }

    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool NetworkManager::cbCheckAvailable(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    return true;
}

bool NetworkManager::cbGetProperties(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    NetworkManager *self = (NetworkManager*) user_data;

    LSMessageRef(message);

    return self->processGetPropertiesMethod(lshandle, message);
}

bool NetworkManager::cbSetProperty(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    return true;
}

bool NetworkManager::cbGetTechnologies(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    return true;
}

bool NetworkManager::cbGetServices(LSHandle* lshandle, LSMessage *message, void *user_data)
{
    return true;
}
