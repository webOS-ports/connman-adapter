/*
 * @@@LICENSE
 *
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
 *
 * Some of the code is fairly copied from libconnman-qt under the terms of the Apache 2.0
 * license and is
 * Copyright (c) 2010, Intel Corporation.
 * Copyright (c) 2012, Jolla.
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

#include "wifiservice.h"
#include "connmanagent.h"
#include "utilities.h"

#define WIFI_TECHNOLOGY_NAME    "wifi"
#define AGENT_PATH              "/WifiSettings"

#define MAX_SIGNAL_BARS         5

static LSMethod _serviceMethods[]  = {
    { "getstatus", WifiNetworkService::cbGetStatus },
    { "setstate", WifiNetworkService::cbSetState },
    { "findnetworks", WifiNetworkService::cbFindNetworks },
    { "connect", WifiNetworkService::cbConnect },
    { "getprofile", WifiNetworkService::cbGetProfile },
    { "getinfo", WifiNetworkService::cbGetInfo },
    { "deleteprofile", WifiNetworkService::cbDeleteProfile },
    { "getprofilelist", WifiNetworkService::cbGetProfileList },
    { 0, 0 }
};

WifiNetworkService::WifiNetworkService(QObject *parent) :
    QObject(parent),
    _manager(NULL),
    _wifiTechnology(NULL),
    _currentService(NULL),
    _agent(this)
{
    _manager = NetworkManagerFactory::createInstance();

    connect(_manager, SIGNAL(availabilityChanged(bool)),
            this, SLOT(managerAvailabilityChanged(bool)));
    connect(_manager, SIGNAL(technologiesChanged(QMap<QString, NetworkTechnology*>, QStringList)),
            this, SLOT(updateTechnologies(QMap<QString, NetworkTechnology*>, QStringList)));

    _wifiTechnology = _manager->getTechnology(WIFI_TECHNOLOGY_NAME);
    if (_wifiTechnology) {
        connect(_wifiTechnology, SIGNAL(poweredChanged(bool)),
                this, SIGNAL(wifiPoweredChanged(bool)));
    }

    QDBusConnection::systemBus().registerObject(AGENT_PATH, this);
    _manager->registerAgent(QString(AGENT_PATH));
}

WifiNetworkService::~WifiNetworkService()
{
}

void WifiNetworkService::start(LSPalmService *service)
{
    int ret;
    LSError lserror;

    LSErrorInit(&lserror);

    _privateService = LSPalmServiceGetPrivateConnection(service);

    ret = LSPalmServiceRegisterCategory(service, "/",
                NULL, _serviceMethods, NULL, this, &lserror);
    if (!ret) {
        g_error("Failed to register service category /");
        return;
    }

    LSErrorFree(&lserror);
}

void WifiNetworkService::updateTechnologies(const QMap<QString, NetworkTechnology*> &added, const QStringList &removed)
{
    QString wifiTechType = QString(WIFI_TECHNOLOGY_NAME);
    if (added.contains(wifiTechType)) {
        _wifiTechnology = added.value(wifiTechType);
        connect(_wifiTechnology, SIGNAL(poweredChanged(bool)),
                this, SLOT(wifiPoweredChanged(bool)));
    }
    else if (removed.contains(wifiTechType)) {
        _wifiTechnology = NULL; // FIXME: is it needed?
    }
}

void WifiNetworkService::managerAvailabilityChanged(bool available)
{
    /* FIXME disable service and send signals */
}

void WifiNetworkService::wifiPoweredChanged(bool powered)
{
    json_object *response;
    const char *payload;
    LSError lserror;

    LSErrorInit(&lserror);

    if (!powered &&
        _currentService != NULL &&
        (_stateOfCurrentService == READY || _stateOfCurrentService == ONLINE)) {
        sendConnectionStatusToSubscribers("notAssociated");
    }

    _currentService = NULL;
    _stateOfCurrentService = IDLE;

    response = json_object_new_object();
    json_object_object_add(response, "returnValue", json_object_new_boolean(true));
    json_object_object_add(response, "status",
        json_object_new_string(powered ? "serviceEnabled" : "serviceDisabled"));
    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    payload = json_object_to_json_string(response);

    if (!LSSubscriptionPost(_privateService, "/", "getstatus", payload, &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    /* FIXME should we post to public subscribers as well? */

    if (response && !is_error(response))
        json_object_put(response);
}

bool WifiNetworkService::checkForConnmanService(json_object *response)
{
    if (!_manager->isAvailable()) {
        qDebug() << "Connman service is not available; returning with error!";

        /* FIXME error codes are unknown right now so sending 1 as default */
        json_object_object_add(response, "errorCode", json_object_new_int(1));
        json_object_object_add(response, "errorText", json_object_new_string("Connman service is not availalbe"));
        return false;
    }

    return true;
}

QList<NetworkService*> WifiNetworkService::listNetworks() const
{
    const QString wifiTypeName(WIFI_TECHNOLOGY_NAME);
    QList<NetworkService*> networks;

    foreach(NetworkService *network, _manager->getServices()) {
        if (network->type() == wifiTypeName)
            networks.append(network);
    }

    return networks;
}

bool WifiNetworkService::isWifiPowered() const
{
    if (_wifiTechnology)
        return _wifiTechnology->powered();
    return false;
}

bool WifiNetworkService::setWifiPowered(const bool &powered)
{
    if (_wifiTechnology)
        _wifiTechnology->setPowered(powered);
}

void WifiNetworkService::currentServiceStateChanged(const QString &changedState)
{
    int newState;
    QString palmState;
    LSError lserror;

    LSErrorInit(&lserror);

    newState = parse_connman_service_state(_currentService->state().toUtf8().constData());

    palmState = convert_connman_service_state_to_palm(newState, _stateOfCurrentService);

    qDebug() << "currentServiceStateChanged: palmState = " << palmState << " state = " << changedState;

    if (newState == CONFIGURATION && _connectServiceRequest.valid) {
        /* We're now successfully associated with the network so we can complete the
         * connect request from the user. */
        json_object_object_add(_connectServiceRequest.response, "returnValue",
            json_object_new_boolean(true));

        if (!LSMessageReply(_connectServiceRequest.handle, _connectServiceRequest.message,
                json_object_to_json_string(_connectServiceRequest.response), &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }

        json_object_put(_connectServiceRequest.response);

        /* That means we can take the service as new profile as well */
        if (_profiles.findProfileByDBusPath(_currentService->dbusPath()) != NULL) {
            ServiceProfile *profile = _profiles.createProfile(_currentService->dbusPath());
            qDebug() << "New profile: service = " << profile->dbusPath() << " id = " << profile->id();
        }

        _connectServiceRequest.reset();
    }

    sendConnectionStatusToSubscribers(palmState);

    _stateOfCurrentService = newState;
}

void WifiNetworkService::appendConnectionStatusToMessage(json_object *message, NetworkService *service, const QString& state)
{
    json_object *networkInfo;
    json_object *ipInfo;
    json_object *apInfo;
    QVariantMap ipInfoMap;
    QStringList nameserverList;
    QString nameserver;
    ServiceProfile *profile;

    json_object_object_add(message, "status", json_object_new_string("connectionStateChanged"));

    networkInfo = json_object_new_object();

    profile = _profiles.findProfileByDBusPath(service->dbusPath());
    if (profile != NULL) {
        json_object_object_add(networkInfo, "profileId", json_object_new_int(profile->id()));
    }

    json_object_object_add(networkInfo, "ssid",
        json_object_new_string(service->name().toUtf8().constData()));
    json_object_object_add(networkInfo, "securityType", json_object_new_string(""));
    json_object_object_add(networkInfo, "connectState", json_object_new_string(state.toUtf8().constData()));
    json_object_object_add(networkInfo, "signalBars",
            json_object_new_int((service->strength() * MAX_SIGNAL_BARS) / 100));
    json_object_object_add(networkInfo, "signalLevel", json_object_new_int(service->strength()));
    json_object_object_add(networkInfo, "lastConnectError", json_object_new_string(""));

    json_object_object_add(message, "networkInfo", networkInfo);

    if (state == "ipConfigured") {
        ipInfo = json_object_new_object();

        /* FIXME we need to determine the interface via the technology API */
        json_object_object_add(ipInfo, "interface", json_object_new_string("wlan0"));

        ipInfoMap = service->ipv4();
        json_object_object_add(ipInfo, "ip", json_object_new_string(ipInfoMap["Address"].toByteArray().constData()));
        json_object_object_add(ipInfo, "subnet", json_object_new_string(ipInfoMap["Netmask"].toByteArray().constData()));
        json_object_object_add(ipInfo, "gateway", json_object_new_string(ipInfoMap["Gateway"].toByteArray().constData()));

        /* pick first nameserver from the list as it's the one currently used */
        nameserverList = ipInfoMap["Nameservers"].toStringList();
        if (!nameserverList.isEmpty()) {
            nameserver = nameserverList.first();

            json_object_object_add(ipInfo, "dns1",
                json_object_new_string(nameserver.toUtf8().constData()));
        }

        json_object_object_add(message, "ipInfo", ipInfo);
    }
}

void WifiNetworkService::sendConnectionStatusToSubscribers(const QString& state)
{
    json_object *serviceStatus;
    const char *payload;
    LSError lserror;

    serviceStatus = json_object_new_object();

    json_object_object_add(serviceStatus, "returnValue", json_object_new_boolean(true));
    appendConnectionStatusToMessage(serviceStatus, _currentService, state);

    payload = json_object_to_json_string(serviceStatus);

    if (!LSSubscriptionPost(_privateService, "/", "getstatus", payload, &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(serviceStatus);
}

bool WifiNetworkService::connectWithSsid(const QString& ssid, json_object *request, json_object *response)
{
    json_object *wasCreatedWithJoinOther;
    json_object *security;
    json_object *securityType;
    json_object *ssidObj;
    json_object *simpleSecurity;
    json_object *enterpriseSecurity;
    json_object *passKey;
    json_object *keyIndex;
    bool success = false;

    /* Ok, we have several cases to handle here:
     * 1. Open
     * 2. WEP
     * 3. Pre-shared key
     * 4. Enterprise networks */

    /* We want to connect to an open network without any security enabled */
    foreach (NetworkService *service, listNetworks()) {
        if (service->name() == ssid) {
            /* Be sure we're not yet connected to the network */
            if (service->state() != "idle" && service->state() != "failure") {
                json_object_object_add(response, "errorText",
                    json_object_new_string("Trying to connect to a network not in idle state"));
                break;
            }

            if (_currentService != NULL) {
                /* Don't get any signals from former connected network anymore */
                disconnect(_currentService, SIGNAL(stateChanged(const QString&)), this, SLOT(currentServiceStateChanged(const QString&)));
            }

            _currentService = service;
            _stateOfCurrentService = parse_connman_service_state(_currentService->state().toUtf8().constData());

            connect(_currentService, SIGNAL(stateChanged(const QString&)), this, SLOT(currentServiceStateChanged(const QString&)));

            _connectionSettings.reset();

            wasCreatedWithJoinOther = json_object_object_get(request, "wasCreatedWithJoinOther");
            if (wasCreatedWithJoinOther) {
                _connectionSettings.hiddenNetwork = json_object_get_boolean(wasCreatedWithJoinOther);
            }

            ssidObj = json_object_object_get(request, "ssid");
            if (!ssidObj) {
                json_object_object_add(response, "errorText", json_object_new_string("No ssid provided to connect to network"));
                break;
            }
            _connectionSettings.name = json_object_get_string(ssidObj);

            security = json_object_object_get(request, "security");
            if (security) {
                securityType = json_object_object_get(security, "securityType");
                _connectionSettings.setupFromPalmSecurityType(QString(json_object_get_string(securityType)));

                if (_connectionSettings.securityType == ConnectionSettings::WEP ||
                    _connectionSettings.securityType == ConnectionSettings::PSK) {
                    simpleSecurity = json_object_object_get(security, "simpleSecurity");
                    if (!simpleSecurity) {
                        json_object_object_add(response, "errorText",
                            json_object_new_string("Indicated simple security type but no settings provided"));
                        break;
                    }

                    passKey = json_object_object_get(simpleSecurity, "passKey");
                    if (!passKey) {
                        json_object_object_add(response, "errorText",
                            json_object_new_string("No passkey for network security provided"));
                        break;
                    }

                    /* FIXME take isInHex parameter in advance too */
                    _connectionSettings.passphrase = json_object_get_string(passKey);

                    if (_connectionSettings.securityType == ConnectionSettings::WEP) {
                        keyIndex = json_object_object_get(simpleSecurity, "keyIndex");
                        if (!keyIndex) {
                            json_object_object_add(response, "errorText",
                                json_object_new_string("No key index provided but needed"));
                            break;
                        }

                        _connectionSettings.keyIndex = json_object_get_int(keyIndex);
                    }
                }
                else if (_connectionSettings.securityType == ConnectionSettings::IEEE8021x)
                {
                    json_object_object_add(response, "errorText",
                        json_object_new_string("Networks with enterprise security are not support yet"));
                    break;
                }
            }

            /* Any further work is handled by the agent instance we connected to connman */
            _currentService->requestConnect();
            success = true;
            break;
        }
    }

    return success;
}

bool WifiNetworkService::connectWithProfileId(int id, json_object *response)
{
    ServiceProfile *profile;
    bool success = false;

    profile = _profiles.findProfileById(id);
    if (profile == NULL) {
        json_object_object_add(response, "errorText", json_object_new_string("Invalid profile id provided"));
        return false;
    }

    foreach(NetworkService *service, listNetworks()) {
        if (service->dbusPath() == profile->dbusPath()) {
            _currentService = service;
            _stateOfCurrentService = parse_connman_service_state(_currentService->state().toUtf8().constData());
            _connectionSettings.reset();

            connect(_currentService, SIGNAL(stateChanged(const QString&)), this, SLOT(currentServiceStateChanged(const QString&)));
            _currentService->requestConnect();

            success = true;
            break;
        }
    }

    return success;
}

void WifiNetworkService::provideInputForConnman(const QVariantMap& fields, const QDBusMessage& message)
{
    QDBusMessage reply = message.createReply();
    QDBusMessage error;
    QVariantMap responseFields;

    /* FIXME check provided service path with the one we're connecting to */

    if (fields.contains("Passphrase")) {
        responseFields.insert("Passphrase", QVariant(_connectionSettings.passphrase));
    }

    /* We're only providing a network name and not the binary ssid id */
    if (fields.contains("Name")) {
        responseFields.insert("Name", QVariant(_connectionSettings.name));
    }

    if (fields.contains("Identity") || fields.contains("Username") || fields.contains("Password")) {
        error = message.createErrorReply(QString("net.connman.Agent.Error.Canceled"),
            QString("Enterprise networks are not supported yet"));
        QDBusConnection::systemBus().send(error);
        return;
    }

    reply << responseFields;
    QDBusConnection::systemBus().send(reply);
}

void WifiNetworkService::processErrorFromConnman(const QString& error)
{
    LSError lserror;

    LSErrorInit(&lserror);

    if (_connectServiceRequest.valid) {
        json_object_object_add(_connectServiceRequest.response, "returnValue",
            json_object_new_boolean(false));

        json_object_object_add(_connectServiceRequest.response, "errorText",
            json_object_new_string(error.toUtf8().constData()));

        if (!LSMessageReply(_connectServiceRequest.handle, _connectServiceRequest.message,
                json_object_to_json_string(_connectServiceRequest.response), &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }

        json_object_put(_connectServiceRequest.response);
    }
}

bool WifiNetworkService::processGetStatusMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lserror;
    bool subscribed = false;
    bool success = false;
    QString state;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (LSMessageIsSubscription(message)) {
        if (!LSSubscriptionProcess(handle, message, &subscribed, &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }

        json_object_object_add(response, "subscribed", json_object_new_boolean(subscribed));
    }

    if (!checkForConnmanService(response))
        goto done;

    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(response, "status",
        json_object_new_string(isWifiPowered() ? "serviceEnabled" : "serviceDisabled"));

    if (isWifiPowered() && _currentService != NULL) {
        state = convert_connman_service_state_to_palm(_stateOfCurrentService, _stateOfCurrentService);
        appendConnectionStatusToMessage(response, _currentService, state);
    }

    success = true;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processSetStateMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *root;
    json_object *state;
    QString stateValue;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    const char* str = LSMessageGetPayload(message);
    if( !str )
        return false;

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    root = json_tokener_parse(str);
    if (!root || is_error(root)) {
        root = 0;
        json_object_object_add(response, "errorText", json_object_new_string("InvalidRequest"));
        goto done;
    }

    state = json_object_object_get(root, "state");
    stateValue = json_object_get_string(state);

    if (stateValue.isEmpty() || (stateValue != "enabled" && stateValue != "disabled")) {
        json_object_object_add(response, "errorCode", json_object_new_int(1));
        json_object_object_add(response, "errorText", json_object_new_string("InvalidStateValue"));
        goto done;
    }

    if (stateValue == "enabled" && isWifiPowered()) {
        json_object_object_add(response, "errorCode", json_object_new_int(15));
        json_object_object_add(response, "errorText", json_object_new_string("AlreadyEnabled"));
        goto done;
    }
    else if (stateValue == "disabled" && !isWifiPowered()) {
        success = true;
        goto done;
    }

    setWifiPowered(stateValue == "enabled" ? true : false);

    success = true;

done:
    if (root)
        json_object_put(root);

    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    LSMessageReply(handle, message, json_object_to_json_string(response), &lserror);

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processFindNetworksMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *foundNetworks;
    json_object *network;
    json_object *networkInfo;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    if (!isWifiPowered()) {
        json_object_object_add(response, "errorCode", json_object_new_int(12));
        json_object_object_add(response, "errorText", json_object_new_string("NotPermitted"));
        goto done;
    }

    /* FIXME should be done asynchronously */
    _wifiTechnology->scan();

    foundNetworks = json_object_new_array();
    foreach(NetworkService *service, this->listNetworks()) {
        QString connectState = "";
        QString securityTypeValue = "none";
        ServiceProfile *profile = NULL;
        network = json_object_new_object();
        networkInfo = json_object_new_object();

        profile = _profiles.findProfileByDBusPath(service->dbusPath());
        if (profile != NULL) {
            json_object_object_add(networkInfo, "profileId", json_object_new_int(profile->id()));
        }
        else if (service->favorite()) {
            profile = _profiles.createProfile(service->dbusPath());
            qDebug() << "New profile: service = " << profile->dbusPath() << " id = " << profile->id();

            json_object_object_add(networkInfo, "profileId", json_object_new_int(profile->id()));
        }

        /* default values needed for each entry */
        json_object_object_add(networkInfo, "ssid",
            json_object_new_string(service->name().toUtf8().constData()));

        if (!service->security().isEmpty()) {
            securityTypeValue = service->security().first();
        }

        json_object_object_add(networkInfo, "securityType",
            json_object_new_string(convert_connman_security_type_to_palm(securityTypeValue.toUtf8().constData())));

        /* We only get a normalized value for the signal strength in range of 0-100 from
         * connman so we have to convert it here to map it to com.palm.wifi API */
        json_object_object_add(networkInfo, "signalBars",
            json_object_new_int((service->strength() * MAX_SIGNAL_BARS) / 100));
        json_object_object_add(networkInfo, "signalLevel", json_object_new_int(service->strength()));

        if (service->state() == "failure")
            /* FIXME we can't differ between "ipFailed" and "associationFailed" here; need
             * to track service state somehow. */
            connectState = "ipFailed";
        else if (service->state() == "association")
            connectState = "associating";
        else if (service->state() == "online")
            connectState = "ipConfigured";

        if (!connectState.isEmpty()) {
            json_object_object_add(networkInfo, "connectState",
                json_object_new_string(connectState.toUtf8().constData()));
        }

        json_object_object_add(network, "networkInfo", networkInfo);
        json_object_array_add(foundNetworks, network);
    }

    json_object_object_add(response, "foundNetworks", foundNetworks);
    success = true;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processConnectMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *request;
    json_object *profileId;
    json_object *ssid;
    json_object *securityType;
    QString ssidValue;
    QString securityTypeValue;
    LSError lserror;
    const char *payload;
    bool success = false;
    int profileIdValue;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    payload = LSMessageGetPayload(message);
    if( !payload )
        return false;

    request = json_tokener_parse(payload);
    if (!request || is_error(request)) {
        request = 0;
        json_object_object_add(response, "errorText", json_object_new_string("InvalidRequest"));
        goto done;
    }

    profileId = json_object_object_get(request, "profileId");
    ssid = json_object_object_get(request, "ssid");

    if (profileId && ssid) {
        json_object_object_add(response, "errorText", json_object_new_string("Only profileId OR ssid as parameter is allowed"));
        goto done;
    }
    else if (profileId && securityType) {
        json_object_object_add(response, "errorText", json_object_new_string("Parameter securityType is not allowed when profileId is specified"));
        goto done;
    }

    if (profileId) {
        profileIdValue = json_object_get_int(profileId);
        qDebug() << "Connecting with profile id ...";
        success = connectWithProfileId(profileIdValue, response);
    }
    else if (ssid) {
        ssidValue = json_object_get_string(ssid);
        qDebug() << "Connecting with ssid ...";
        success = connectWithSsid(ssidValue, request, response);
    }

done:
    if (!success) {
        json_object_object_add(response, "returnValue", json_object_new_boolean(success));

        if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }

        json_object_put(response);
    }
    else {
        _connectServiceRequest.reset();
        _connectServiceRequest.handle = handle;
        _connectServiceRequest.message = message;
        _connectServiceRequest.response = response;
        _connectServiceRequest.valid = true;

        /* FIXME issue a short timeout to be sure our client gets a response */
    }

    if (request)
        json_object_put(request);

    return true;
}

bool WifiNetworkService::processGetProfileMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processGetInfoMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *wifiinfo;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    wifiinfo = json_object_new_object();

    /* default values until we have something real */
    json_object_object_add(wifiinfo, "macAddress", json_object_new_string("ff:ff:ff:ff:ff:ff"));
    json_object_object_add(wifiinfo, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "wmm", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "roaming", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "powerSave", json_object_new_string("enabled"));

    json_object_object_add(response, "wifiInfo", wifiinfo);

    success = true;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processDeleteProfileMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processGetProfileListMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lserror;
    bool success = false;

    LSErrorInit(&lserror);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    if (!LSMessageReply(handle, message, json_object_to_json_string(response), &lserror)) {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    json_object_put(response);

    return true;
}

#define LS2_CB_METHOD(name) \
bool WifiNetworkService::cb##name(LSHandle* lshandle, LSMessage *message, void *user_data) \
{ \
    WifiNetworkService *self = (WifiNetworkService*) user_data; \
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
