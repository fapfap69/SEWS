# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(SEWS C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Opzioni di compilazione
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_COVERAGE "Enable coverage reporting" ON)

# Trova le dipendenze
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Aggiungi i flag per la coverage se abilitata
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
endif()

# Aggiungi i sorgenti principali
file(GLOB SOURCES "src/*.c")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

# Crea la libreria principale
add_library(sews_lib ${SOURCES})
target_link_libraries(sews_lib 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

# Crea l'eseguibile principale
add_executable(sews src/main.c)
target_link_libraries(sews sews_lib)

# Configurazione dei test
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES "tests/*.c")
    
    add_executable(run_tests ${TEST_SOURCES})
    target_link_libraries(run_tests sews_lib)
    
    add_test(NAME unit_tests COMMAND run_tests)
endif()

# Installazione
install(TARGETS sews
    RUNTIME DESTINATION bin
)

install(DIRECTORY www/
    DESTINATION share/sews/www
)
