find_package(Qt5 5.9 REQUIRED COMPONENTS Core Network Test)

# now prepare the qt version for usage in our CMakeLists
get_target_property (QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)

# execute qmake to get version
execute_process(COMMAND ${QT_QMAKE_EXECUTABLE} -version OUTPUT_VARIABLE QT_VERSION_RAW)
string(REGEX MATCH "[0-9]\.[0-9]+\.[0-9]+" QT_VERSION ${QT_VERSION_RAW})

# status output
message(STATUS "...using '${QT_QMAKE_EXECUTABLE}'.  Qt version: ${QT_VERSION}")

# extract version number parts
string(REPLACE "." ";" QT_VERSION_LIST ${QT_VERSION})
list(GET QT_VERSION_LIST 0 QT_VERSION_MAJOR)
list(GET QT_VERSION_LIST 1 QT_VERSION_MINOR)
list(GET QT_VERSION_LIST 2 QT_VERSION_PATCH)

# per default: we enable auto-mocing and include those generated Qt files automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
