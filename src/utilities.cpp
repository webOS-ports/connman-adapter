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

#include "utilities.h"

json_object *convertQVariantToJsonObject(const QVariant &v)
{
    json_object *result = NULL;

    switch(v.type()) {
    case QVariant::Bool:
        result = json_object_new_boolean(v.toBool());
        break;
    case QVariant::Int:
        result = json_object_new_int(v.toInt());
        break;
    case QVariant::Double:
        result = json_object_new_double(v.toDouble());
        break;
    case QVariant::String:
        result = json_object_new_string(v.toString().toUtf8().data());
        break;
    case QVariant::Map:
    case QVariant::List:
        /* FIXME */
        break;
    }

    return result;
}