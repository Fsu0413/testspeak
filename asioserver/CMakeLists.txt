
project(testspeak)
cmake_minimum_required(VERSION 3.1)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
pkg_check_modules(jsoncpp REQUIRED jsoncpp)
endif()

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Possible type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(SRC_LIST
  main.cpp
  server.cpp
  server.h
)

add_definitions(-DASIO_STANDALONE)
	
add_executable(${PROJECT_NAME} ${SRC_LIST})
include_directories(${CMAKE_SOURCE_DIR}/include)
add_compile_options(${jsoncpp_CFLAGS})
target_link_libraries(${PROJECT_NAME} ${jsoncpp_LIBRARIES})
link_directories(${jsoncpp_LIBRARY_DIRS})

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if(MINGW)
        set(CMAKE_CXX_FLAGS_RELEASE "-Wl,-s ${CMAKE_CXX_FLAGS_RELEASE}")
    endif()

    if(MSVC)
        # Changing flags for static linking.
        foreach(flag_var CMAKE_CXX_FLAGS
                         CMAKE_CXX_FLAGS_RELEASE
                         CMAKE_CXX_FLAGS_MINSIZEREL)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
    endif()
	
	target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
