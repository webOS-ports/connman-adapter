cmake_minimum_required (VERSION 2.8.7)
project (connman-adapter CXX)

include(webOS/webOS)

webos_modules_init(1 0 0 QUALIFIER RC3)
webos_component(0 1 0)
webos_machine_impl_dep()

include(FindPkgConfig)
include(CheckLibraryExists)
include(CheckFunctionExists)

pkg_check_modules(GLIB_2 REQUIRED glib-2.0)
include_directories(${GLIB_2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB_2_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

pkg_check_modules(QT_CORE REQUIRED QtCore)
include_directories(${QT_CORE_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${QT_CORE_CFLAGS_OTHER})

pkg_check_modules(QT_DBUS REQUIRED QtDBus)
include_directories(${QT_DBUS_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${QT_DBUS_CFLAGS_OTHER})

pkg_check_modules(CONNMAN_QT REQUIRED connman-qt4)
include_directories(${CONNMAN_QT_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${CONNMAN_QT_CFLAGS_OTHER})

include_directories(src/)

file(GLOB SOURCE_FILES src/*.cpp)

webos_build_system_bus_files()

add_executable(connman-adapter ${SOURCE_FILES})

target_link_libraries(connman-adapter
    -lpthread
    ${GLIB_2_LDFLAGS}
    ${QT_CORE_LDFLAGS}
    ${QT_DBUS_LDFLAGS}
    ${LS2_LDFLAGS}
    ${CONNMAN_QT_LDFLAGS})

webos_build_daemon()
