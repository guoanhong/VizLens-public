cmake_minimum_required(VERSION 2.8)
project( viz )

include_directories(/usr/local/include)
link_directories(/usr/lib)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set( OpenCV_DIR ../../opencv-2.4.10/build )
# set( OpenCV_FOUND 1 )
find_package( OpenCV REQUIRED )

include_directories(/usr/local/Cellar/jsoncpp/1.8.4/include)
link_directories(/usr/local/Cellar/jsoncpp/1.8.4/lib)
# find_package( JSONCPP REQUIRED )
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(JSONCPP jsoncpp)
# link_libraries(${JSONCPP_LIBRARIES})

# include_directories("${JSONCPP_INCLUDE_DIRS}")
# pkg_check_modules(JSONCPP jsoncpp)
# link_libraries(${JSONCPP_LIBRARIES})

# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}"${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# find_package (sqlite3 REQUIRED )
# include_directories(${SQLITE3_INCLUDE_DIRS})
# target_link_libraries (tutorial ${SQLITE3_LIBRARIES})

# set(CURL_DIR usr/local/curl/ )
find_package(curl REQUIRED)
# include_directories(${CURL_INCLUDE_DIR})
# link_directories(${CURL_LIBRARY_DIRS})

# set( LEPTONICA_DIR leptonica/build )
# set( Leptonica_DIR leptonica/build )
# find_package( Leptonica REQUIRED ) 
# include_directories( ${Leptonica_INCLUDE_DIRS} )
# link_directories( ${LEPTONICA_LIBRARY_DIRS} )


# set( Tesseract_DIR tesseract/build )
# find_package( Tesseract REQUIRED )
# include_directories( ${Tesseract_INCLUDE_DIRS} )
# link_directories( ${TESSERACT_LIBRARY_DIRS} )

file(GLOB ACCESS
    main_access.cpp
    lib/access/db_utils.hpp
    lib/access/db_utils.cpp
    lib/access/img_utils.hpp
    lib/access/img_utils.cpp
    lib/access/feedback_utils.hpp
    lib/access/feedback_utils.cpp
    lib/access/statemap_utils.hpp
    lib/access/statemap_utils.cpp
    lib/access/visualization.hpp
    lib/access/visualization.cpp
)
add_executable( vizaccess ${ACCESS})
target_link_libraries( vizaccess ${OpenCV_LIBS} )
target_link_libraries( vizaccess ${CURLPP_LIBRARIES} )
target_link_libraries( vizaccess ${CURL_LIBRARIES})
target_link_libraries( vizaccess sqlite3 )

file(GLOB BUILDSOURCES
    main_build.cpp
    lib/cpp-base64/base64.h
    lib/cpp-base64/base64.cpp
    lib/db_utils.hpp
    lib/db_utils.cpp
    lib/img_utils.hpp
    lib/img_utils.cpp
    lib/ocr_utils.hpp
    lib/ocr_utils.cpp
    lib/feedback_utils.hpp
    lib/feedback_utils.cpp
    lib/statemap_utils.hpp
    lib/statemap_utils.cpp
    lib/visualization.hpp
    lib/visualization.cpp
)
add_executable( vizbuild ${BUILDSOURCES})
target_link_libraries( vizbuild ${OpenCV_LIBS} )
target_link_libraries( vizbuild ${CURLPP_LIBRARIES} )
target_link_libraries( vizbuild ${CURL_LIBRARIES})
target_link_libraries( vizbuild ${JSONCPP_LIBRARIES})
target_link_libraries( vizbuild sqlite3 )
