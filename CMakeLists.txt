# This is a basic example of an OMNeT++ project using CMake.
# Compared to more advanced projects, this example depends only on OMNeT++ itself.
project(simulation)
# Alternatively, you can also explicitly request a particular OMNeT++ version:

# Tell CMake where it can the omnetpp-cmake sources
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set(OMNETPP_ROOT /home/wld/.libs/omnetpp-5.6.2/)
set(OMNETPP_INCLUDE_DIR ${OMNETPP_ROOT}include/)
set(OMNETPP_MSGC ${OMNETPP_ROOT}bin/opp_msgc)

find_package(OmnetPP 5.6 REQUIRED)

include(GenerateOppMessage)
include(ImportOppTarget)

set(INET_DIR /home/wld/workspace/omnet/inet)
find_path(INET_DIR NAMES /home/wld/workspace/omnet/inet/ DOC "INET root directory")
import_opp_target(INET ${INET_DIR}/src/Makefile)

add_subdirectory(examples/basic)
add_subdirectory(examples/pcaprecorder)
add_subdirectory(examples/e-voting-setup)
add_subdirectory(examples/e-voting-execution)
add_subdirectory(examples/did-e-voting-setup)