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

#include "wifiservice.h"
#include "utilities.h"

#define WIFI_TECHNOLOGY_NAME    "wifi"

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

/**
 * TODO:
 * - return an error whenever a luna method is called and connman service is not available
 **/

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
    /* FIXME check wether wifi technology disappeared */
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
        goto done;
    }

    state = json_object_object_get(root, "state");
    stateValue = json_object_get_string(state);

    if (stateValue.isEmpty() || (stateValue != "enabled" && stateValue != "disabled"))
        goto done;

    /* stateValue can now be only disabled or enabled */
    setWifiPowered((stateValue == "enabled"));

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
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

done:
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
    LSError lsError;

    LSErrorInit(&lsError);

    response = json_object_new_object();

    if (!checkForConnmanService(response))
        goto done;

    json_object_object_add(response, "returnValue", json_object_new_boolean(true));

    /* default values until we have something real */
    json_object_object_add(response, "macAddress", json_object_new_string("ff:ff:ff:ff:ff:ff"));
    json_object_object_add(response, "wakeOnWlan", json_object_new_string("disabled"));
    json_object_object_add(response, "wmm", json_object_new_string("disabled"));
    json_object_object_add(response, "roaming", json_object_new_string("disabled"));
    json_object_object_add(response, "powerSave", json_object_new_string("enabled"));

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
