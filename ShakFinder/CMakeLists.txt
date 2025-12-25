
include(FetchContent)
# fetch shakkar/fast-reachability repo
FetchContent_Declare(
	fast_reachability
	GIT_REPOSITORY https://github.com/shakkar23/fast-reachability.git
	GIT_TAG main
)

FetchContent_MakeAvailable(fast_reachability Fast_Reachability)

set(SHAKFINDER_SOURCES
	"ShakFinder.cpp"
 "Solver/Parser.cpp" "Solver/Solver.cpp" )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_executable (ShakFinder ${SHAKFINDER_SOURCES})

target_link_libraries(ShakFinder Fast_Reachability)

# set to 23 when available
set_property(TARGET ShakFinder PROPERTY CXX_STANDARD 23)
