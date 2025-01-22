cmake_minimum_required(VERSION 3.10.0)
project(linear-pbft-server VERSION "0.1.0" LANGUAGES CXX)
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

find_package(OpenSSL REQUIRED)
if(NOT OPENSSL_FOUND)
  message(FATAL_ERROR "OpenSSL not found. Please install OpenSSL or set the OpenSSL path.")
endif()

if(APPLE)
  set(CRYPTOPP_ROOT_DIR "/opt/homebrew/opt/cryptopp")
  set(CRYPTOPP_INCLUDE_DIR "${CRYPTOPP_ROOT_DIR}/include")
  set(CRYPTOPP_LIBRARIES "${CRYPTOPP_ROOT_DIR}/lib/libcryptopp.a")
elseif(UNIX AND NOT APPLE)
set(CRYPTOPP_ROOT_DIR "/usr")
set(CRYPTOPP_INCLUDE_DIR "${CRYPTOPP_ROOT_DIR}/include")
set(CRYPTOPP_LIBRARIES "${CRYPTOPP_ROOT_DIR}/lib/x86_64-linux-gnu/libcryptopp.a")
endif()

find_library(CRYPTOPP_LIB cryptopp PATHS /usr/local/lib /usr/lib)
if(CRYPTOPP_LIB)
  message(STATUS "Found Crypto++ library: ${CRYPTOPP_LIB}")
  set(CRYPTOPP_LIBRARIES ${CRYPTOPP_LIB})
else()
  message(FATAL_ERROR "Crypto++ library not found!")
endif()

# MCL
# set(MCL_INCLUDE_DIR "/usr/local/include")
# set(MCL_LIB "/usr/local/lib/libmcl.a")

# if(NOT MCL_INCLUDE_DIR OR NOT MCL_LIB)
#     message(FATAL_ERROR "MCL library not found. Please set MCL_DIR or install MCL in a standard location.")
# endif()

# include_directories(${MCL_INCLUDE_DIR})
# link_libraries(${MCL_LIB})

# Setup Executable
add_executable(linear-pbft-server
  "src/server.cpp"
  "include/server.h"
  "src/globals.cpp"
  "src/utils.cpp"
  "lib/pbft_client_service_impl.cpp"
  "lib/pbft_protocol_service_impl.cpp"
  "lib/workers/commit_worker.cpp"
  "lib/workers/execute_worker.cpp"
  "lib/workers/prepare_worker.cpp"
  "lib/workers/prepared_worker.cpp"
  "lib/workers/preprepare_worker.cpp"
  "lib/workers/request_worker.cpp"
  "lib/workers/new_view_worker.cpp"
  "lib/workers/view_change_worker.cpp"
  "models/client_data.cpp"
  "models/log_store.cpp"
  "models/server_state_data.cpp"
)

target_include_directories(linear-pbft-server PRIVATE ${CRYPTOPP_INCLUDE_DIR})
target_include_directories(linear-pbft-server PRIVATE "include")
target_include_directories(linear-pbft-server PRIVATE ${OPENSSL_INCLUDE_DIR})

target_link_libraries(linear-pbft-server
  linear_pbft_grpc_proto
  # ${MCL_LIB}
  OpenSSL::SSL 
  OpenSSL::Crypto
  ${CRYPTOPP_LIBRARIES}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)