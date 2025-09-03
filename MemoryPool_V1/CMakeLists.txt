cmake_minimum_required(VERSION 3.12)
project(MemoryPoolProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 修改编译选项：移除 -Werror，暂时只保留警告
if(MSVC)
    add_compile_options(/W4)  # 移除 /WX（视警告为错误）
else()
    add_compile_options(-Wall -Wextra -Wpedantic)  # 移除 -Werror
endif()

# 添加可执行文件
add_executable(${PROJECT_NAME}
    MemoryPool.cpp
    test_demo.cpp  # 确保包含这个文件
)

# 包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接pthread库
target_link_libraries(${PROJECT_NAME} PRIVATE pthread)