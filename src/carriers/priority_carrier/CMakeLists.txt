# Copyright (C) 2012 IITRBCS
# Authors: Ali Paikan and Paul Fitzpatrick
# CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT

yarp_prepare_plugin(priority
                    CATEGORY carrier
                    TYPE yarp::os::PriorityCarrier
                    INCLUDE PriorityCarrier.h
                    EXTRA_CONFIG SUBTYPE=recv
                                 CODE="not applicable"
                    DEPENDS "CREATE_OPTIONAL_CARRIERS"
                    DEFAULT ON)

if(NOT SKIP_priority)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)

  get_property(YARP_OS_INCLUDE_DIRS TARGET YARP_OS PROPERTY INCLUDE_DIRS)
  get_property(YARP_sig_INCLUDE_DIRS TARGET YARP_sig PROPERTY INCLUDE_DIRS)
  include_directories(${YARP_OS_INCLUDE_DIRS}
                      ${YARP_sig_INCLUDE_DIRS})
  if(CREATE_LIB_MATH)
    get_property(YARP_math_INCLUDE_DIRS TARGET YARP_math PROPERTY INCLUDE_DIRS)
    include_directories(${YARP_math_INCLUDE_DIRS})
    add_definitions(-DWITH_YARPMATH)
  endif()

  yarp_add_plugin(yarp_priority PriorityCarrier.h
                                PriorityCarrier.cpp)
  target_link_libraries(yarp_priority YARP_OS
                                      YARP_sig)
  if(CREATE_LIB_MATH)
     target_link_libraries(yarp_priority YARP_math)
  endif()

  yarp_install(TARGETS yarp_priority
               EXPORT YARP
               COMPONENT runtime
               LIBRARY DESTINATION ${YARP_DYNAMIC_PLUGINS_INSTALL_DIR}
               ARCHIVE DESTINATION ${YARP_STATIC_PLUGINS_INSTALL_DIR})
  yarp_install(FILES priority.ini
               COMPONENT runtime
               DESTINATION ${YARP_PLUGIN_MANIFESTS_INSTALL_DIR})
endif()
