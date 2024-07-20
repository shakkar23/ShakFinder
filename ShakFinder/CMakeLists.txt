# CMakeList.txt : CMake project for ShakFinder, include source and define
# project specific logic here.
#
add_subdirectory("ShakTrisLib")


set(SHAKFINDER_SOURCES
	"ShakFinder.cpp"
 "Solver/Parser.cpp" "Solver/Solver.cpp" )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_executable (ShakFinder ${SHAKFINDER_SOURCES})

target_link_libraries(ShakFinder ShakTris)

# set to 23 when available
set_property(TARGET ShakFinder PROPERTY CXX_STANDARD 23)