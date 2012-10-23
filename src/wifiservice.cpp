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
#include "utilities.h"

#define WIFI_TECHNOLOGY_NAME    "wifi"

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
    _manager(false)
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
}

WifiNetworkService::~WifiNetworkService()
{
}

void WifiNetworkService::start(LSPalmService *service)
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
}

bool WifiNetworkService::checkForConnmanService(json_object *response)
{
    if (!_manager->isAvailable()) {
        qDebug() << "Connman service is not available; returning with error!";

        json_object_object_add(response, "returnValue", json_object_new_boolean(false));
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

bool WifiNetworkService::processGetStatusMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    json_object_object_add(response, "returnValue", json_object_new_boolean(true));
    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(response, "status",
        json_object_new_string(isWifiPowered() ? "serviceEnabled" : "serviceDisabled"));

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processSetStateMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *root;
    json_object *state;
    QString stateValue;
    LSError lsError;
    bool success = false;

    LSErrorInit(&lsError);

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
        json_object_object_add(response, "errorCode", json_object_new_int(15));
        json_object_object_add(response, "errorText", json_object_new_string("AlreadyDisabled"));
        goto done;
    }

    setWifiPowered(stateValue == "enabled" ? true : false);

    success = true;

done:
    if (root)
        json_object_put(root);

    json_object_object_add(response, "returnValue", json_object_new_boolean(success));

    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    json_object_put(response);

    return true;
}

bool WifiNetworkService::processFindNetworksMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *foundNetworks;
    json_object *networkInfo;
    json_object *availableSecurityTypes;
    LSError lsError;
    bool success = false;

    LSErrorInit(&lsError);

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
        availableSecurityTypes = json_object_new_array();
        networkInfo = json_object_new_object();

        /* default values needed for each entry */
        json_object_object_add(networkInfo, "ssid",
            json_object_new_string(service->name().toUtf8().constData()));

        /* connman only provides the security implementation here but no details about the
         * key management. This way we can't map the connman types to the old style
         * com.palm.wifi API types. */
        foreach(QString securityType, service->security()) {
            json_object_array_add(availableSecurityTypes,
                json_object_new_string(securityType.toUtf8().constData()));
        }

        json_object_object_add(networkInfo, "availableSecurityTypes", availableSecurityTypes);

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

        json_object_array_add(foundNetworks, networkInfo);
    }

    json_object_object_add(response, "foundNetworks", foundNetworks);
    success = true;

done:
    json_object_object_add(response, "returnCode", json_object_new_boolean(success));

    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processConnectMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processGetProfileMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processGetInfoMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    json_object *wifiinfo;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    json_object_object_add(response, "returnValue", json_object_new_boolean(true));

    wifiinfo = json_object_new_object();

    /* default values until we have something real */
    json_object_object_add(wifiinfo, "macAddress", json_object_new_string("ff:ff:ff:ff:ff:ff"));
    json_object_object_add(wifiinfo, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "wmm", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "roaming", json_object_new_string("disabled"));
    json_object_object_add(wifiinfo, "powerSave", json_object_new_string("enabled"));

    json_object_object_add(response, "wifiInfo", wifiinfo);

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processDeleteProfileMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

    return true;
}

bool WifiNetworkService::processGetProfileListMethod(LSHandle *handle, LSMessage *message)
{
    json_object *response;
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
    LSMessageReply(handle, message, json_object_to_json_string(response), &lsError);

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
