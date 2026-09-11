cmake_minimum_required(VERSION 3.16)
project(TreeCacheDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)

add_library(treecache_lib STATIC
    src/cache/Cache.cpp
    src/cache/Cache.h
    src/cache/TrackingState.h
    src/cache/TreeCacheElement.h
    src/db/ChangeSet.h
    src/db/CommitResult.h
    src/db/Database.cpp
    src/db/Database.h
    src/db/TreeDbElement.h
    src/domain/Id.h
    src/domain/TreeElement.h
    src/sample/SampleTree.cpp
    src/sample/SampleTree.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
target_include_directories(treecache_lib PUBLIC src)
target_link_libraries(treecache_lib PUBLIC Qt5::Widgets)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(treecache_lib PRIVATE -Wall -Wextra)
endif()

add_executable(TreeCacheDemo src/main.cpp)
target_link_libraries(TreeCacheDemo PRIVATE treecache_lib)
