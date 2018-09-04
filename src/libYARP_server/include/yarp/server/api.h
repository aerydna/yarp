/*
* Copyright (C) 2006-2018 Istituto Italiano di Tecnologia (IIT)
* Copyright (C) 2006-2010 RobotCub Consortium
* All rights reserved.
*
* This software may be modified and distributed under the terms of the
* BSD-3-Clause license. See the accompanying LICENSE file for details.
*/

#ifndef YARP_SERVER_API_H
#define YARP_SERVER_API_H

#include <yarp/conf/api.h>
#ifndef YARP_server_API
#  ifdef YARP_server_EXPORTS
#    define YARP_server_API YARP_EXPORT
#    define YARP_server_EXTERN YARP_EXPORT_EXTERN
#  else
#    define YARP_server_API YARP_IMPORT
#    define YARP_server_EXTERN YARP_IMPORT_EXTERN
#  endif
#  define YARP_server_DEPRECATED_API YARP_DEPRECATED_API
#  define YARP_server_DEPRECATED_API_MSG(X) YARP_DEPRECATED_API_MSG(X)
#endif

#endif // YARP_SERVER_API_H
