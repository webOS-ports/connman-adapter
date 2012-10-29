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

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <QVariant>
#include <cjson/json.h>

#define CONNMAN_SERVICE_STATE_IDLE              1
#define CONNMAN_SERVICE_STATE_ASSOCIATION       2
#define CONNMAN_SERVICE_STATE_CONFIGURATION     3
#define CONNMAN_SERVICE_STATE_READY             4
#define CONNMAN_SERVICE_STATE_ONLINE            5
#define CONNMAN_SERVICE_STATE_DISCONNECT        6
#define CONNMAN_SERVICE_STATE_FAILURE           7

json_object *convertQVariantToJsonObject(const QVariant &v);
int parse_connman_service_state(const char* state);
char* convert_connman_security_type_to_palm(const char* type);
char* convert_connman_service_state_to_palm(int state, int last_state);

#endif
