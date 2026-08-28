#设置Cmake最低版本为3.10

cmake_minimum_required(VERSION 3.10)
#设置项目名称
project(Test)
#设置C++标准
set(CMAKE_CXX_STANDARD 17)
#设置C++标准警告
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#设置构建类型
set(CMAKE_BUILD_TYPE Debug)
#添加可执行文件
add_executable(TestLLM TestLLM.cpp  ../sdk/src/ChatgptProvider.cpp ../sdk/src/util/my_spdlog.cpp ../sdk/src/ollamaProvider.cpp)

#设置输出目录
set_target_properties(TestLLM PROPERTIES EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build)
#添加头文件搜索路径
target_include_directories(TestLLM PRIVATE ../sdk/include)
find_package(OpenSSL REQUIRED)
target_include_directories(TestLLM PRIVATE ${OPENSSL_INCLUDE_DIR})

#链接库
target_link_libraries(TestLLM jsoncpp fmt spdlog gtest OpenSSL::SSL OpenSSL::Crypto pthread
    dl)
#设置CPPhttplib支持https
target_compile_definitions(TestLLM PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)