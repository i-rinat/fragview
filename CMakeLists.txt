project(fragview)
cmake_minimum_required(VERSION 2.6)
set(CMAKE_C_FLAGS_DEBUG "-ggdb3 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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
