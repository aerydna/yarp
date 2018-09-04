# Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
# Copyright (C) 2006-2010 RobotCub Consortium
# All rights reserved.
#
# This software may be modified and distributed under the terms of the
# BSD-3-Clause license. See the accompanying LICENSE file for details.

project(YARP_server)

set(YARP_server_SRCS src/yarpserver.cpp)

set(YARP_server_HDRS include/yarp/server/yarpserver.h
                     include/yarp/server/api.h)

add_library(YARP_server ${YARP_server_SRCS}
                        ${YARP_server_HDRS})

add_library(YARP::YARP_server ALIAS YARP_server)

target_include_directories(YARP_server PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                              $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(YARP_server PUBLIC YARP::YARP_conf
                                            YARP::YARP_OS
                                     PRIVATE YARP::YARP_name
                                             YARP::YARP_init)
list(APPEND YARP_server_PUBLIC_DEPS YARP_conf
                                       YARP_OS)
list(APPEND YARP_server_PRIVATE_DEPS YARP_name)

set_property(TARGET YARP_server PROPERTY PUBLIC_HEADER ${YARP_server_HDRS})
set_property(TARGET YARP_server PROPERTY VERSION ${YARP_VERSION_SHORT})
set_property(TARGET YARP_server PROPERTY SOVERSION ${YARP_SOVERSION})

install(TARGETS YARP_server
        EXPORT YARP_server
        COMPONENT YARP_server
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/yarp/server")

include(YarpInstallBasicPackageFiles)
yarp_install_basic_package_files(YARP_server
                                 DEPENDENCIES ${YARP_server_PUBLIC_DEPS}
                                 PRIVATE_DEPENDENCIES ${YARP_server_PRIVATE_DEPS})

set_property(TARGET YARP_server PROPERTY FOLDER "Libraries/Private")
