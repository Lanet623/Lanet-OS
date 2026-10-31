cmake_minimum_required(VERSION 3.16)
project(lanet-shell C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)

add_executable(lanet-shell src/main.c)
target_include_directories(lanet-shell PRIVATE ${GTK3_INCLUDE_DIRS} ${GIO_UNIX_INCLUDE_DIRS})
target_link_libraries(lanet-shell PRIVATE ${GTK3_LIBRARIES} ${GIO_UNIX_LIBRARIES})
target_compile_options(lanet-shell PRIVATE ${GTK3_CFLAGS_OTHER} ${GIO_UNIX_CFLAGS_OTHER})

add_executable(lanet-settings src/settings.c)
target_include_directories(lanet-settings PRIVATE ${GTK3_INCLUDE_DIRS} ${GIO_UNIX_INCLUDE_DIRS})
target_link_libraries(lanet-settings PRIVATE ${GTK3_LIBRARIES} ${GIO_UNIX_LIBRARIES})
target_compile_options(lanet-settings PRIVATE ${GTK3_CFLAGS_OTHER} ${GIO_UNIX_CFLAGS_OTHER})

install(TARGETS lanet-shell RUNTIME DESTINATION bin)
install(TARGETS lanet-settings RUNTIME DESTINATION bin)
install(FILES data/lanet-shell.css DESTINATION share/lanet-shell)
install(FILES data/lanet-shell.desktop data/lanet-settings.desktop DESTINATION share/applications)
install(FILES data/sway-lanet.conf DESTINATION share/lanet-shell)
