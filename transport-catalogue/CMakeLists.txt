cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_PREFIX_PATH "/usr/bin/Protobuf")
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

# Proto files
set(PROTO_FILES 
        "svg.proto"
        "map_renderer.proto"
        "transport_catalogue.proto"
        "graph.proto"
        "transport_router.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Project files
set(TRANSPORT_CATALOGUE_FILES
        "domain.h"          "domain.cpp"
        "geo.h"             "geo.cpp"
        "json.h"            "json.cpp"
        "json_builder"      "json_builder.cpp"
        "json_reader.h"     "json_reader.cpp"
        "json.h"            "json.cpp"
        "map_renderer.h"    "map_renderer.cpp"
        "ranges.h"
        "request_handler.h" "request_handler.cpp"
        "router.h"
        "serialization.h"   "serialization.cpp"
        "svg.h"             "svg.cpp"
        "transport_catalogue.h" "transport_catalogue.cpp"
        "transport_router.h" "transport_router.cpp"
        "main.cpp")

# Target executable
add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${PROTO_FILES} ${TRANSPORT_CATALOGUE_FILES})

# find_package определила переменную Protobuf_INCLUDE_DIRS,
# которую нужно использовать как include-путь.
# Также нужно добавить как include-путь директорию, куда
# protoc положит сгенерированные файлы.
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
# файлы .h и .сс сформируются в директории build (она же CMAKE_BINARY_DIR - переменная в кэше)
target_include_directories(transport_catalogue PUBLIC ${CMAKE_BINARY_DIR})

# Специальные методы корректировки ответов cmake... трюк какой-то
string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

# Целевая библиотека
target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)