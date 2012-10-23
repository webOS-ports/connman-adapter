connman-adapter
===============

Summary
-------

Open webOS component to interface with the connman daemon. It's providing the
com.palm.wifi service API with a connman based backend. All concepts from connman are
mapped to the com.palm.wifi API.

How to Build on Linux
=====================

## Dependencies

Below are the tools and libraries (and their minimum versions) required to build connman-adapter:

* cmake (version required by openwebos/cmake-modules-webos)
* g++ 4.6.3
* glib-2.0 2.32.1
* make (any version)
* qt 4.8.3
* openwebos/luna-service2 3.0.0
* pkg-config 0.26
* connman-qt4 0.1.4

## Building

Once you have downloaded the source, enter the following to build it (after
changing into the directory under which it was downloaded):

    $ qmake
    $ make
    $ sudo make install

## Uninstalling

From the directory where you originally ran `make install`, enter:

    $ [sudo] make uninstall

You will need to use `sudo` if you did not specify `WEBOS_INSTALL_ROOT`.

# Copyright and License Information

Unless otherwise specified, all content, including all source code files and
documentation files in this repository are:

Copyright (c) 2012 Simon Busch <morphis@gravedo.de>

Unless otherwise specified or set forth in the NOTICE file, all content,
including all source code files and documentation files in this repository are:
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this content except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
