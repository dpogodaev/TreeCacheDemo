cmake_minimum_required(VERSION 3.16)
project(TreeCacheDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core)

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
)
target_include_directories(treecache_lib PUBLIC src)
target_link_libraries(treecache_lib PUBLIC Qt5::Core)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(treecache_lib PRIVATE -Wall -Wextra)
endif()