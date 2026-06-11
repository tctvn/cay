cmake_minimum_required(VERSION 3.22.1)

project("cayime")

set(CORE_DIR "../../../../../../core")

add_library(cayime SHARED
        telex_jni.cpp
        ${CORE_DIR}/CayEngine.cpp
        ${CORE_DIR}/CayData.cpp
)

target_include_directories(cayime PRIVATE ${CORE_DIR})

find_library(log-lib log)

target_link_libraries(cayime ${log-lib})
