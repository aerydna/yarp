/*
 * Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include <yarp/serversql/yarpserversql.h>

#include <yarp/server/yarpserver.h>
#include <string>

int main(int argc, char *argv[]) {
    
    if (argc > 1 && std::string(argv[1]) == "--new")
    {
        yarp::server::NameServer y;
        y.start();
    }
    else
    {
        int ret = yarpserver_main(argc, argv);
        return (ret != 0 ? 1 : 0);
    }
}
