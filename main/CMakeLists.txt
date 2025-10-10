cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)

set(LIGHTGBM_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/include")

if (WIN32)
    # Windows: MSVC builds put static lib in Release/ by default
    set(LIGHTGBM_STATIC_LIB_FILE
        "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/Release/lib_lightgbm.lib")
else()
    # Linux / macOS
    message(FATAL_ERROR "Only compiles on Windows")
    set(LIGHTGBM_STATIC_LIB_FILE
        "${CMAKE_CURRENT_LIST_DIR}/../../LightGBM/lib_lightgbm.a")
endif()

set(SRC_FILES "")
list(APPEND SRC_FILES
    src/main.cpp
    include/efeum/driver1/config.h
                src/driver1/config.cpp
    include/efeum/driver1/communication.h
                src/driver1/communication.cpp
    include/efeum/scanning/scan.h
                        src/scan.cpp
    include/efeum/ml/lgbm.h
                        src/lgbm.cpp
    include/efeum/mmapping.h
                        src/mmapping.cpp
    include/efeum/helpers.h
                        src/helpers.cpp
)

add_executable(my_program ${SRC_FILES})

target_include_directories(my_program PUBLIC include)
target_include_directories(my_program PUBLIC ${LIGHTGBM_INCLUDE_DIR})

set(LINKED_LIBRARIES "${LIGHTGBM_STATIC_LIB_FILE}")

if (NOT WIN32)
    list(APPEND LINKED_LIBRARIES
        pthread      # required on Linux
        m            # math library
        rt           # real-time library
        dl           # dynamic loader
    )
endif()

if (WIN32)
    target_link_libraries(my_program PRIVATE
        user32
        kernel32
    )
endif()

find_package(OpenMP REQUIRED)

if(OpenMP_CXX_FOUND)
    list(APPEND LINKED_LIBRARIES OpenMP::OpenMP_CXX)
endif()

target_link_libraries(my_program PUBLIC ${LINKED_LIBRARIES})

target_link_libraries(my_program PUBLIC efe_core)


# mio - cross-platform memory-mapping library
# https://github.com/vimpunk/mio

set(MIO_GIT_URL "https://github.com/vimpunk/mio.git")
set(MIO_VERSION "8b6b7d878c89e81614d05edca7936de41ccdd2da")

FetchContent_Declare(mio
    GIT_REPOSITORY  "${MIO_GIT_URL}"
    GIT_TAG         "${MIO_VERSION}"
)

FetchContent_MakeAvailable(mio)

target_link_libraries(my_program PUBLIC mio::mio)

## RE2

target_link_libraries(my_program PUBLIC re2::re2)
