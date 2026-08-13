cmake_minimum_required(VERSION 3.10)
project(CayHelper)

set(CMAKE_CXX_STANDARD 17)

add_executable(cayhelper
    main.cpp
    ../shared/InputInjector.cpp
)
target_link_libraries(cayhelper setupapi)

add_executable(test_cay
    test_cay.cpp
    ../../../core/CayEngine.cpp
    ../../../core/CayData.cpp
)
