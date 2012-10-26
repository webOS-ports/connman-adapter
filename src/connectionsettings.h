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

#ifndef CONNECTIONSETTINGS_H_
#define CONNECTIONSETTINGS_H_

class ConnectionSettings
{
public:
    ConnectionSettings()
        : hiddenNetwork(false),
          name(""),
          passphrase(""),
          securityType(NONE)
    {
    }

    ~ConnectionSettings()
    {
    }

    void setupFromPalmSecurityType(const QString& type)
    {
        if (type == "wep") {
            securityType =  WEP;
        }
        else if (type == "wpa-personal" || type == "wapi-psk") {
            securityType = PSK;
        }
        else if (type == "enterprise" || type == "wapi-cert" ) {
            securityType = IEEE8021x;
        }
    }

    void reset()
    {
        securityType = NONE;
        hiddenNetwork = false;
        name = "";
        passphrase = "";
    }

    enum SecurityType {
        NONE,
        WEP,
        PSK,
        IEEE8021x,
        WPS,
    };

    bool hiddenNetwork;
    int keyIndex;
    QString name;
    QString passphrase;
    SecurityType securityType;
};

#endif
