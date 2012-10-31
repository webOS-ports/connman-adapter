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

#ifndef SERVICEPROFILE_H_
#define SERVICEPROFILE_H_

class ServiceProfile
{
public:
    ServiceProfile(NetworkService *service, int id)
        : _service(service),
          _id(id)
    {
    }

    ~ServiceProfile() { }

    QString dbusPath()
    {
        return _service->dbusPath();
    }

    int id()
    {
        return _id;
    }

    NetworkService* service() {
        return _service;
    }

private:
    NetworkService *_service;
    int _id;
};


class ServiceProfileList
{
public:
    ServiceProfileList() : _lastProfileId(1) { }
    ~ServiceProfileList() { }

    ServiceProfile* createProfile(NetworkService *service)
    {
        ServiceProfile *profile = new ServiceProfile(service, _lastProfileId++);
        _profiles.append(profile);
        return profile;
    }

    ServiceProfile* findProfileById(int id)
    {
        ServiceProfile *profile = NULL;

        foreach (ServiceProfile *currentProfile, _profiles) {
            if (currentProfile->id() == id) {
                profile = currentProfile;
                break;
            }
        }

        return profile;
    }

    ServiceProfile* findProfileByDBusPath(const QString& path)
    {
        ServiceProfile *profile = NULL;

        foreach (ServiceProfile *currentProfile, _profiles) {
            if (currentProfile->dbusPath() == path) {
                profile = currentProfile;
                break;
            }
        }

        return profile;
    }

    void removeProfileById(int id)
    {
        ServiceProfile *profileToRemove = NULL;

        foreach (ServiceProfile *currentProfile, _profiles) {
            if (currentProfile->id() == id) {
                profileToRemove = currentProfile;
                break;
            }
        }

        if (profileToRemove != NULL) {
            _profiles.removeOne(profileToRemove);
            delete profileToRemove;
        }
    }

    const QList<ServiceProfile*> list()
    {
        return _profiles;
    }

private:
    int _lastProfileId;
    QList<ServiceProfile*> _profiles;
};

#endif
