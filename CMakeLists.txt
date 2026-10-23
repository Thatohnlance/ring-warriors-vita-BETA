cmake_minimum_required(VERSION 3.10)
project(RingWarriors C)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK environment variable is not set. Build with VitaSDK.")
endif()
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_APP_NAME "Ring Warriors")
set(VITA_TITLEID "RWAR00001")
set(VITA_VERSION "01.00")
add_executable(${PROJECT_NAME}
  src/main.c
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -Wall -Wextra)
target_link_libraries(${PROJECT_NAME} vita2d SceDisplay_stub SceCtrl_stub SceGxm_stub SceSysmodule_stub SceCommonDialog_stub ScePower_stub m)
vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME "${VITA_APP_NAME}"
)
