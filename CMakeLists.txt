cmake_minimum_required(VERSION 3.20)
project(DanjonSimulator LANGUAGES CXX)

# 1. On désactive la gestion automatique du standard par CMake pour éviter l'erreur
# set(CMAKE_CXX_STANDARD 26) # <-- NE PAS UTILISER ÇA, ça fait planter ton CMake

# 2. On définit nos fichiers sources
file(GLOB BACK_SOURCES "./back/core/*.cpp" "./back/data/*.cpp" "./back/systems/*.cpp")
file(GLOB BINDING_SOURCES "./back/bindings/*.cpp")
file(GLOB FRONT_SOURCES "./front/src/*.cpp" "./front/include/*.hpp")
add_executable(DanjonSimulator main.cpp ${BACK_SOURCES} ${BINDING_SOURCES} ${FRONT_SOURCES})

# Qt6 Configuration
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
target_link_libraries(DanjonSimulator PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
set_target_properties(DanjonSimulator PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)

target_include_directories(DanjonSimulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/back/deps)
target_include_directories(DanjonSimulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/front/include)

# 3. On force manuellement le flag C++26 (latest) pour MSVC
if(MSVC)
    target_compile_options(DanjonSimulator PRIVATE /std:c++latest)
    # Optionnel : active aussi les rapports de fonctionnalités pour vérifier
    target_compile_definitions(DanjonSimulator PRIVATE _HAS_CXX26=1)
else()
    # Si jamais tu passes sur GCC ou Clang plus tard
    target_compile_options(DanjonSimulator PRIVATE -std=c++26)
endif()

# 4. On ajoute le binaire de tests unitaires (sans Qt)
file(GLOB TEST_SOURCES "./tests/*.cpp")
add_executable(test_core ${TEST_SOURCES} ${BACK_SOURCES})
target_include_directories(test_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/back/deps)
if(MSVC)
    target_compile_options(test_core PRIVATE /std:c++latest)
    target_compile_definitions(test_core PRIVATE _HAS_CXX26=1)
else()
    target_compile_options(test_core PRIVATE -std=c++26)
endif()