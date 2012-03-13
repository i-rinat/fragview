project (fragview)
cmake_minimum_required (VERSION 2.6)
set (CMAKE_C_FLAGS_DEBUG "-ggdb3 -O0")
set (CMAKE_CXX_FLAGS_DEBUG "-ggdb3 -O0")
set (CMAKE_C_FLAGS_RELEASE "-O3")
set (CMAKE_CXX_FLAGS_RELEASE "-O3")
if(NOT CMAKE_BUILD_TYPE)
	set (CMAKE_BUILD_TYPE Release)
endif()

find_package (PkgConfig REQUIRED)
find_package (Boost REQUIRED)

pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules (GLIBMM REQUIRED glibmm-2.4)
pkg_check_modules (SQLITE3 REQUIRED sqlite3)

link_directories (
	${GTKMM_LIBRARY_DIRS}
	${SQLITE3_LIBRARY_DIRS}
)

include_directories (
	${GTKMM_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
)

add_library (clusters STATIC
	clusters.cc
	util.cc
)

add_executable (fragview
	fragview.cc
	fragmap-widget.cc
	filelist-widget.cc
	mountpoint-select-dialog.cc
)

add_executable (fileseverity
	fileseverity.cc
)

target_link_libraries (fragview
	${GTKMM_LIBRARIES}
	clusters
)

target_link_libraries (fileseverity
	${GLIBMM_LIBRARIES}
	clusters
)

add_executable (fragdb
	fragdb.cc
)

target_link_libraries (fragdb
	${GLIBMM_LIBRARIES}
	${SQLITE3_LIBRARIES}
	clusters
)

install (TARGETS fileseverity fragdb fragview
	RUNTIME DESTINATION "bin"
)
