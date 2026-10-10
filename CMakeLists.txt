cmake_minimum_required(VERSION 3.16)
project(TRIGGER LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)

add_executable(trigger
    src/main.cc
    src/DetectorConstruction.cc
    src/PrimaryGeneratorAction.cc
)

target_include_directories(trigger PRIVATE include)
target_link_libraries(trigger PRIVATE ${Geant4_LIBRARIES})

file(COPY macros DESTINATION ${CMAKE_CURRENT_BINARY_DIR})