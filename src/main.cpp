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

#include <QCoreApplication>

#include "servicemgr.h"

static GMainLoop *mainloop;

int main(int argc, char **argv)
{
    int ret;
    GMainContext *ctx;
    ServiceManager smgr;

    QCoreApplication app(argc, argv);

    ctx = g_main_context_default();
    mainloop = g_main_loop_new(ctx, TRUE);

    smgr.start(mainloop);

    app.exec();

    smgr.stop();

    g_main_loop_unref(mainloop);
    g_main_context_unref(ctx);

    return 0;
}
