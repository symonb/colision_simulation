cmake_minimum_required(VERSION 3.5)
project(colision_simulation) # this does not matter for our simple case
set(TARGET "simulation")  # "simulation" determines built file name
add_executable(${TARGET}  
src/main.cpp    # add each of source files
src/ball.cpp
)
find_package(benchmark REQUIRED)
find_package(SFML 2.4 COMPONENTS system window graphics REQUIRED)
find_package( OpenCV REQUIRED )
find_package(TBB REQUIRED)

target_include_directories(${TARGET} PUBLIC include)
include_directories( ${OpenCV_INCLUDE_DIRS} )
target_compile_features(${TARGET} PUBLIC cxx_std_20) # we need C++20

target_link_libraries(${TARGET} PUBLIC benchmark::benchmark)
target_link_libraries(${TARGET} PUBLIC sfml-graphics)
target_link_libraries(${TARGET} PUBLIC sfml-window)
target_link_libraries(${TARGET} PUBLIC sfml-system)
target_link_libraries(${TARGET} PUBLIC  ${OpenCV_LIBS} )
target_link_libraries(${TARGET} PUBLIC TBB::tbb)


