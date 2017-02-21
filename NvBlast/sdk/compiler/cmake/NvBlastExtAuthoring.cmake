#
# Build NvBlastExt Common
#


SET(COMMON_SOURCE_DIR ${PROJECT_SOURCE_DIR}/common)

SET(AUTHORING_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoring/source)
SET(COMMON_EXT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extensions/common/source)
SET(AUTHORING_EXT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/extensions/authoring/include)

FIND_PACKAGE(PxSharedSDK $ENV{PM_PxShared_VERSION} REQUIRED)
FIND_PACKAGE(PhysXSDK $ENV{PM_PhysX_VERSION} REQUIRED)

# Include here after the directories are defined so that the platform specific file can use the variables.
include(${PROJECT_CMAKE_FILES_DIR}/${TARGET_BUILD_PLATFORM}/NvBlastExtAuthoring.cmake)

SET(COMMON_FILES
	${BLASTEXT_PLATFORM_COMMON_FILES}
	
	#${COMMON_SOURCE_DIR}/NvBlastAssert.cpp
	#${COMMON_SOURCE_DIR}/NvBlastAssert.h
)

SET(PUBLIC_FILES
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringBondGenerator.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringCollisionBuilder.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringFractureTool.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringMesh.h
	${AUTHORING_EXT_INCLUDE_DIR}/NvBlastExtAuthoringTypes.h
)

SET(EXT_AUTHORING_FILES
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringAccelerator.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringAccelerator.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBondGenerator.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBooleanTool.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringBooleanTool.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringCollisionBuilder.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringMesh.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringPerlinNoise.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringTriangulator.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringTriangulator.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringVSA.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringFractureTool.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtTriangleProcessor.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtTriangleProcessor.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtApexSharedParts.cpp
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtApexSharedParts.h
	${AUTHORING_EXT_SOURCE_DIR}/NvBlastExtAuthoringInternalCommon.h	
)

ADD_LIBRARY(NvBlastExtAuthoring STATIC 
	${COMMON_FILES}
	${PUBLIC_FILES}

	${EXT_AUTHORING_FILES}
)

SOURCE_GROUP("common" FILES ${COMMON_FILES})
SOURCE_GROUP("public" FILES ${PUBLIC_FILES})
SOURCE_GROUP("src" FILES ${EXT_AUTHORING_FILES})


# Target specific compile options

TARGET_INCLUDE_DIRECTORIES(NvBlastExtAuthoring 
	PRIVATE ${BLASTEXT_PLATFORM_INCLUDES}

	PUBLIC ${PROJECT_SOURCE_DIR}/lowlevel/include
	PUBLIC ${AUTHORING_EXT_INCLUDE_DIR}

	PRIVATE ${PROJECT_SOURCE_DIR}/common
	PRIVATE ${COMMON_EXT_SOURCE_DIR}
	
	PRIVATE ${AUTHORING_EXT_SOURCE_DIR}

	PRIVATE ${PHYSXSDK_INCLUDE_DIRS}
	PRIVATE ${PXSHAREDSDK_INCLUDE_DIRS}
)

TARGET_COMPILE_DEFINITIONS(NvBlastExtAuthoring
	PRIVATE ${BLASTEXT_COMPILE_DEFS}
)

# Warning disables for Capn Proto
TARGET_COMPILE_OPTIONS(NvBlastExtAuthoring
	PRIVATE ${BLASTEXT_PLATFORM_COMPILE_OPTIONS}
)

SET_TARGET_PROPERTIES(NvBlastExtAuthoring PROPERTIES 
	PDB_NAME_DEBUG "NvBlastExtAuthoring${CMAKE_DEBUG_POSTFIX}"
	PDB_NAME_CHECKED "NvBlastExtAuthoring${CMAKE_CHECKED_POSTFIX}"
	PDB_NAME_PROFILE "NvBlastExtAuthoring${CMAKE_PROFILE_POSTFIX}"
	PDB_NAME_RELEASE "NvBlastExtAuthoring${CMAKE_RELEASE_POSTFIX}"
)

# Do final direct sets after the target has been defined
TARGET_LINK_LIBRARIES(NvBlastExtAuthoring 
	PRIVATE NvBlast
	PUBLIC ${BLASTEXT_PLATFORM_LINKED_LIBS}
	PUBLIC $<$<CONFIG:debug>:${PHYSX3_LIB_DEBUG}> $<$<CONFIG:debug>:${PHYSX3EXTENSIONS_LIB_DEBUG}> $<$<CONFIG:debug>:${PXFOUNDATION_LIB_DEBUG}>
	PUBLIC $<$<CONFIG:checked>:${PHYSX3_LIB_CHECKED}> $<$<CONFIG:checked>:${PHYSX3EXTENSIONS_LIB_CHECKED}> $<$<CONFIG:checked>:${PXFOUNDATION_LIB_CHECKED}>
	PUBLIC $<$<CONFIG:profile>:${PHYSX3_LIB_PROFILE}> $<$<CONFIG:profile>:${PHYSX3EXTENSIONS_LIB_PROFILE}> $<$<CONFIG:profile>:${PXFOUNDATION_LIB_PROFILE}>
	PUBLIC $<$<CONFIG:release>:${PHYSX3_LIB}> $<$<CONFIG:release>:${PHYSX3EXTENSIONS_LIB}> $<$<CONFIG:release>:${PXFOUNDATION_LIB}>
)
