cmake_minimum_required(VERSION 3.1)
project(fragview LANGUAGES CXX)

# request pthreads and c++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=format-security")

# add missing NDEBUG in RelWithDebugInfo
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -g3")

# generate more debug info for Debug build type
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")

find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)

pkg_check_modules(REQ REQUIRED gtkmm-3.0 glibmm-2.4 sqlite3)

# Musl libc does not include fts.h, and relies on an external implementation.
pkg_check_modules(LIBFTS QUIET libfts)

list(APPEND REQ_LIBRARY_DIRS ${Boost_LIBRARY_DIRS} ${LIBFTS_LIBRARY_DIRS})
list(APPEND REQ_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${LIBFTS_INCLUDE_DIRS})
list(APPEND REQ_LIBRARIES ${Boost_LIBRARIES} ${LIBFTS_LIBRARIES})

link_directories(${REQ_LIBRARY_DIRS})

include_directories(${REQ_INCLUDE_DIRS})

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
