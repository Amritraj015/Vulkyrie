CMAKE_MINIMUM_REQUIRED(VERSION 3.22.2)
PROJECT(VulkyrieEngine VERSION 0.0.1 LANGUAGES CXX)

# Find Vulkan
FIND_PACKAGE(Vulkan REQUIRED)

# If Linux based OS
IF(NOT WIN32)
	# Find X11
	FIND_PACKAGE(X11 REQUIRED)

	#=========================================================================================================
	# - Find XCB
	FIND_PACKAGE(PkgConfig)

	IF(NOT XCB_FIND_COMPONENTS)
		SET(XCB_FIND_COMPONENTS xcb)
	ENDIF()

	INCLUDE(FindPackageHandleStandardArgs)
	SET(XCB_FOUND true)
	SET(XCB_INCLUDE_DIRS "")
	SET(XCB_LIBRARIES "")
	FOREACH(comp ${XCB_FIND_COMPONENTS})
		# component name
		STRING(TOUPPER ${comp} compname)
		STRING(REPLACE "-" "_" compname ${compname})
		# header name
		STRING(REPLACE "xcb-" "" headername xcb/${comp}.h)
		# library name
		SET(libname ${comp})

		PKG_CHECK_MODULES(PC_${comp} QUIET ${comp})

		FIND_PATH(${compname}_INCLUDE_DIR NAMES ${headername}
				HINTS
				${PC_${comp}_INCLUDEDIR}
				${PC_${comp}_INCLUDE_DIRS})

		FIND_LIBRARY(${compname}_LIBRARY NAMES ${libname}
				HINTS
				${PC_${comp}_LIBDIR}
				${PC_${comp}_LIBRARY_DIRS})

		FIND_PACKAGE_HANDLE_STANDARD_ARGS(${comp}
				FOUND_VAR ${comp}_FOUND
				REQUIRED_VARS ${compname}_INCLUDE_DIR ${compname}_LIBRARY)

		MARK_AS_ADVANCED(${compname}_INCLUDE_DIR ${compname}_LIBRARY)

		LIST(APPEND XCB_INCLUDE_DIRS ${${compname}_INCLUDE_DIR})
		LIST(APPEND XCB_LIBRARIES ${${compname}_LIBRARY})

		IF(NOT ${comp}_FOUND)
			SET(XCB_FOUND false)
		ENDIF()
	ENDFOREACH()

	#=========================================================================================================
	# Find X11-XCB
	# use pkg-config to get the directories and then use these values
	# in the FIND_PATH() and FIND_LIBRARY() calls
	PKG_CHECK_MODULES(PKG_X11_XCB QUIET x11-xcb)

	SET(X11_XCB_DEFINITIONS ${PKG_X11_XCB_CFLAGS})

	FIND_PATH(X11_XCB_INCLUDE_DIR NAMES X11/Xlib-xcb.h HINTS ${PKG_X11_XCB_INCLUDE_DIRS})
	FIND_LIBRARY(X11_XCB_LIBRARIES NAMES X11-xcb HINTS ${PKG_X11_XCB_LIBRARY_DIRS})

	INCLUDE(FindPackageHandleStandardArgs)
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(X11_XCB DEFAULT_MSG X11_XCB_LIBRARIES X11_XCB_INCLUDE_DIR)

	MARK_AS_ADVANCED(X11_XCB_INCLUDE_DIR X11_XCB_LIBRARIES)
	#=========================================================================================================
ELSE()
	#If Windows OS

ENDIF()

LIST(REMOVE_DUPLICATES XCB_INCLUDE_DIRS)

INCLUDE_DIRECTORIES(Src)
FILE(GLOB_RECURSE LIBRARY_FILES "Src/*.cpp")
ADD_LIBRARY(${PROJECT_NAME} ${LIBRARY_FILES})
TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC _DEBUG)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:Include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>
    PRIVATE
		${Vulkan_INCLUDE_DIR}
		${X11_INCLUDE_DIR}
		${XCB_INCLUDE_DIRS}
		${X11_XCB_INCLUDE_DIR}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    PUBLIC

    PRIVATE
		${Vulkan_LIBRARIES}
		${X11_LIBRARIES}
		${XCB_LIBRARIES}
		${X11_XCB_LIBRARIES}
)