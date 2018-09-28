
project(testspeak)
cmake_minimum_required(VERSION 3.1)

if(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Possible type of build: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(SRC_LIST
  http_client_utils.cpp 
  http_header.cpp 
  json_reader.cpp 
  json_value.cpp 
  json_writer.cpp 
  main.cpp 
  new_line.cpp 
  tcp_client.cpp 
  json_valueiterator.inl 
  json_tool.h 
  http_client.cpp 
  include/http_client.hpp 
  include/http_client_utils.hpp 
  include/new_line.hpp 
  include/tcp_client.hpp 
  include/json/config.h 
  include/json/features.h 
  include/json/forwards.h 
  include/json/json.h 
  include/json/reader.h 
  include/json/value.h 
  include/json/version.h 
  include/json/writer.h 
  include/json/assertions.h 
  include/json/autolink.h 
)

add_executable(${PROJECT_NAME} ${SRC_LIST})
include_directories(${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    if(MINGW)
        set(CMAKE_CXX_FLAGS_RELEASE "-static -static-libgcc -static-libstdc++ -Wl,-s ${CMAKE_CXX_FLAGS_RELEASE}")
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
