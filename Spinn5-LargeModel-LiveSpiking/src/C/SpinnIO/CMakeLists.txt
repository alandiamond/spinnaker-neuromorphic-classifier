# Copyright: 2015 BABEL project Plymouth University
# in collaboration with Manchester University
# Author: Samantha Adams
# CopyPolicy: Released under the terms of the FreeBSD license.

set(LIBNAME SpinnIO)
project(${LIBNAME})

set(INCLUDES include/Threadable.h include/EIEIOReceiver.h include/EIEIOSender.h include/DatabaseIF.h include/SocketIF.h include/EIEIOMessage.h)
set(SOURCES src/Threadable.cpp src/EIEIOReceiver.cpp src/EIEIOSender.cpp src/DatabaseIF.cpp src/SocketIF.cpp src/EIEIOMessage.cpp)

source_group("Header Files" FILES ${INCLUDES})
source_group("Source Files" FILES ${SOURCES})

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${LIBNAME} ${INCLUDES} ${SOURCES})
target_link_libraries(${LIBNAME} pthread sqlite3)
install(TARGETS ${LIBNAME} DESTINATION lib)


