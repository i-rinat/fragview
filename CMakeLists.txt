project(fragview)
cmake_minimum_required(VERSION 2.6)

# request pthreads and c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=format-security")

# add missing NDEBUG in RelWithDebugInfo
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -g3")

# generate more debug info for Debug build type
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")

find_package (PkgConfig REQUIRED)
find_package (Boost REQUIRED)

pkg_check_modules(REQ REQUIRED gtkmm-3.0 glibmm-2.4 sqlite3)

link_directories(${REQ_LIBRARY_DIRS})

include_directories(
    ${REQ_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

add_library(clusters STATIC
    src/clusters.cc
    src/util.cc
)

add_executable(fragview
    src/fragview.cc
    src/fragmap-widget.cc
    src/filelist-widget.cc
    src/mountpoint-select-dialog.cc
)

add_executable(fileseverity
    src/fileseverity.cc
)

target_link_libraries(fragview
    clusters
    ${REQ_LIBRARIES}
)

target_link_libraries(fileseverity
    clusters
    ${REQ_LIBRARIES}
)

add_executable(fragdb
    src/fragdb.cc
)

target_link_libraries(fragdb
    clusters
    ${REQ_LIBRARIES}
)

install(TARGETS fileseverity fragdb fragview
    RUNTIME DESTINATION "bin"
)
