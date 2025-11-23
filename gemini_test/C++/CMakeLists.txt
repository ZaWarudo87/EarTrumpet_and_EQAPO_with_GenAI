# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 專案名稱
project(GeminiExample)

# 設定 C++ 標準為 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 尋找 vcpkg 安裝的函式庫
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 建立可執行檔
add_executable(main main.cpp)

# 連結函式庫到您的可執行檔
target_link_libraries(main PRIVATE cpr::cpr nlohmann_json::nlohmann_json)