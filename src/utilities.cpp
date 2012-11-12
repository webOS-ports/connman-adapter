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

#include <string.h>
#include "utilities.h"

int parse_connman_service_state(const char* state)
{
    int result = CONNMAN_SERVICE_STATE_IDLE;

    if (!strcmp(state, "idle"))
        result = CONNMAN_SERVICE_STATE_IDLE;
    else if (!strcmp(state, "association"))
        result = CONNMAN_SERVICE_STATE_ASSOCIATION;
    else if (!strcmp(state, "configuration"))
        result = CONNMAN_SERVICE_STATE_CONFIGURATION;
    else if (!strcmp(state, "ready"))
        result = CONNMAN_SERVICE_STATE_READY;
    else if (!strcmp(state, "online"))
        result = CONNMAN_SERVICE_STATE_ONLINE;
    else if (!strcmp(state, "disconnect"))
        result = CONNMAN_SERVICE_STATE_DISCONNECT;
    else if (!strcmp(state, "failure"))
        result = CONNMAN_SERVICE_STATE_FAILURE;

    return result;
}

char* convert_connman_security_type_to_palm(const char* type)
{
    if (!strcmp(type, "psk"))
        return "wpa-personal";
    else if (!strcmp(type, "ieee8021x"))
        return "enterprise";
    else if (!strcmp(type, "wep"))
        return "wep";

    return "none";
}

char* convert_connman_service_state_to_palm(int state, int last_state)
{
    switch (state) {
    case CONNMAN_SERVICE_STATE_DISCONNECT:
    case CONNMAN_SERVICE_STATE_IDLE:
        return "notAssociated";
    case CONNMAN_SERVICE_STATE_ASSOCIATION:
        return "associating";
    case CONNMAN_SERVICE_STATE_CONFIGURATION:
        return "associated";
    case CONNMAN_SERVICE_STATE_READY:
    case CONNMAN_SERVICE_STATE_ONLINE:
        return "ipConfigured";
    case CONNMAN_SERVICE_STATE_FAILURE:
        if (last_state == CONNMAN_SERVICE_STATE_ASSOCIATION)
            return "associationFailed";
        else if (last_state == CONNMAN_SERVICE_STATE_CONFIGURATION)
            return "ipFailed";
        break;
    }

    return "notAssociated";
}

char* convert_connman_service_state_to_palm(int state)
{
    return convert_connman_service_state_to_palm(state, state);
}
