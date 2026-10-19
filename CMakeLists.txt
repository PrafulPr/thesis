cmake_minimum_required(VERSION 3.16)
project(TRIGGER LANGUAGES CXX)

find_package(Geant4 REQUIRED ui_all vis_all)

add_executable(trigger
    src/main.cc
    src/DetectorConstruction.cc
    src/PrimaryGeneratorAction.cc
    src/SiPMArray.cc         #Since we moved the SiPM array out (separate)
    src/Collimator.cc        #Collimator... likewise
    src/CrystalWrapping.cc   #Reflective layer aroudn the crystals... likewise
    src/EventAction.cc       #Stepping action (pre and post)
    src/SteppingAction.cc    #Stepping action (pre and post)
)

target_include_directories(trigger PRIVATE include)
target_link_libraries(trigger PRIVATE ${Geant4_LIBRARIES})

file(COPY macros DESTINATION ${CMAKE_CURRENT_BINARY_DIR})