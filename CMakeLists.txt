# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(SWSWS C)

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
add_library(swsws_lib ${SOURCES})
target_link_libraries(swsws_lib 
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
)

# Crea l'eseguibile principale
add_executable(swsws src/main.c)
target_link_libraries(swsws swsws_lib)

# Installazione
install(TARGETS swsws
    RUNTIME DESTINATION bin
)

install(DIRECTORY www/
    DESTINATION share/swsws/www
)
