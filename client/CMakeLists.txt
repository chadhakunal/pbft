cmake_minimum_required(VERSION 3.10.0)
project(linear-pbft-client VERSION "0.1.0" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# GRPC & Protobuf
# Reference: https://github.com/grpc/grpc/blob/v1.66.0/examples/cpp/route_guide/CMakeLists.txt
find_package(Threads REQUIRED)

option(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
find_program(_PROTOBUF_PROTOC protoc)
else()
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

get_filename_component(linear_pbft_proto "../protos/linear-pbft.proto" ABSOLUTE)
get_filename_component(linear_pbft_proto_path "${linear_pbft_proto}" PATH)

set(linear_pbft_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/linear-pbft.pb.cc")
set(linear_pbft_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/linear-pbft.pb.h")
set(linear_pbft_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/linear-pbft.grpc.pb.cc")
set(linear_pbft_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/linear-pbft.grpc.pb.h")

add_custom_command(
      OUTPUT "${linear_pbft_proto_srcs}" "${linear_pbft_proto_hdrs}" "${linear_pbft_grpc_srcs}" "${linear_pbft_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${linear_pbft_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${linear_pbft_proto}"
      DEPENDS "${linear_pbft_proto}")


include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(linear_pbft_grpc_proto
  ${linear_pbft_grpc_srcs}
  ${linear_pbft_grpc_hdrs}
  ${linear_pbft_proto_srcs}
  ${linear_pbft_proto_hdrs})

target_link_libraries(linear_pbft_grpc_proto
  absl::absl_log
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})


# Setup Executable
find_package(Boost REQUIRED COMPONENTS system filesystem)

# find_path(MCL_INCLUDE_DIR mcl/bn256.hpp)
# find_library(MCL_LIB mcl)

# if(NOT MCL_INCLUDE_DIR OR NOT MCL_LIB)
#     message(FATAL_ERROR "MCL library not found. Please set MCL_DIR or install MCL in a standard location.")
# endif()

# include_directories(${MCL_INCLUDE_DIR})

add_executable(linear-pbft-client
  "src/client.cpp"
  "include/client.h"
  "lib/pbft_client.cpp"
  "lib/server_interface.cpp"
  "lib/transaction_set.cpp"
)

target_include_directories(linear-pbft-client PRIVATE "include")
target_include_directories(linear-pbft-client PRIVATE ${boost_SOURCE_DIR})

target_link_libraries(linear-pbft-client
  linear_pbft_grpc_proto
  Boost::system 
  Boost::filesystem
  # ${MCL_LIB}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)
