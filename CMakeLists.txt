project(ReverseTale-Library)

include(cmake-common/Utils.cmake)
String2List(${CMAKE_PREFIX_PATH} CMAKE_PREFIX_PATH)

file(GLOB_RECURSE sources_Crypto    src/Cryptography/*.cpp     include/Cryptography/*.h)
file(GLOB_RECURSE sources_Tools     src/Tools/*.cpp            include/Tools/*.h)
file(GLOB_RECURSE sources_Game      src/Game/*.cpp             include/Game/*.h)

set(Library_SOURCES
	${sources_Crypto}
	${sources_Tools}
	${sources_Game}
)

# ---[ Load dependencies
find_package(Boost REQUIRED)
find_package(cppformat REQUIRED)

set(THIRD_PARTY_INCLUDE
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIR}
)

include_directories(
    ${PROJECT_BINARY_DIR}
    ${THIRD_PARTY_INCLUDE}
)

# ---[ Setup some definitions
set(DEFINITIONS "")
if (NOT ${DEBUG})
  set(DEFINITIONS "${DEFINITIONS} NDEBUG=1")
endif()

# ---[ Add library as static and add dependencies
add_library(ReverseTale-Library STATIC ${Library_SOURCES})

# ---[ Link all third party libraries
target_link_libraries(ReverseTale-Library
    PUBLIC ${Boost_LIBRARY}
    PUBLIC cppformat
)

# ---[ Other projects include dirs
target_include_directories(ReverseTale-Library
    PUBLIC cppformat
)

# ---[ If windows, add ws2_32 library
if (MSVC)
    target_link_libraries(ReverseTale-Library PUBLIC ws2_32)
endif()

# ---[ Set C++11 standard
set_property(TARGET ReverseTale-Library PROPERTY CXX_STANDARD 11)
target_compile_features(ReverseTale-Library PRIVATE cxx_decltype cxx_nullptr cxx_variadic_templates)

# ---[ Setup output directory and include directories
separate_arguments(THIRD_PARTY_INCLUDE)
set_target_properties(ReverseTale-Library PROPERTIES
    OUTPUT_NAME Library
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib

    INTERFACE_COMPILE_DEFINITIONS "${DEFINITIONS}"
    INTERFACE_INCLUDE_DIRECTORIES "${THIRD_PARTY_INCLUDE}"
)

install(TARGETS ReverseTale-Library
	RUNTIME DESTINATION bin/Library
  	LIBRARY DESTINATION lib/Library
  	ARCHIVE DESTINATION lib/Library
 	EXPORT ReverseTale-Library-targets)

install(EXPORT ReverseTale-Library-targets DESTINATION lib/Library)