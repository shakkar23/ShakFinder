# CMakeList.txt : CMake project for ShakFinder, include source and define
# project specific logic here.
#

# Add source to this project's executable.
set(SHAKFINDER_SOURCES
	"ShakFinder.cpp"
	"Engine/Game.cpp"
	"Engine/Piece.cpp"
 "Engine/Parser.cpp" "Engine/Solver.cpp" )

 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_executable (ShakFinder ${SHAKFINDER_SOURCES})

# set to 23 when available
set_property(TARGET ShakFinder PROPERTY CXX_STANDARD 20)