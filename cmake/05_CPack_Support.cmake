# if no PACKAGE version is set via CLI parameters, set it to development
if (NOT CPACK_PACKAGE_VERSION)
    message(STATUS "! no CPACK_PACKAGE_VERSION parameter seen. Defaulting to development version ${PROJECT_VERSION}")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
endif()

# extract version information to separate numbers
string(REPLACE "." ";" VERSION_LIST ${CPACK_PACKAGE_VERSION})
list(GET VERSION_LIST 0 CPACK_PACKAGE_VERSION_MAJOR)
list(GET VERSION_LIST 1 CPACK_PACKAGE_VERSION_MINOR)
list(GET VERSION_LIST 2 CPACK_PACKAGE_VERSION_PATCH)

# append the git hash to it
set(CPACK_PACKAGE_VERSION_SIMPLE ${CPACK_PACKAGE_VERSION})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION}-${VCS_COMMIT_ID})
message(STATUS "  this is version ${CPACK_PACKAGE_VERSION}.")

# general package options
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Qt5 library implementation of AMQP 0.9.1, focusing on RabbitMQ")
set(CPACK_PACKAGE_NAME "qamqp")
set(CPACK_PACKAGE_VENDOR "Matt Broadstone and contributors")
set(CPACK_PACKAGE_CONTACT mbroadst@gmail.com)

# NOTE: 'include(CPack)'  MUST happen after all CPACK_ variables are set in the including sub directories (e.g. for menu links)
