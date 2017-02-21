/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAPEXIMPORTTOOL_H
#define NVBLASTEXTAPEXIMPORTTOOL_H

#include "NvBlast.h"
#include <vector>
#include <string>
#include "NvBlastExtPxAsset.h"

namespace physx
{
class PxErrorCallback;
class PxAllocatorCallback;
namespace general_PxIOStream2
{
class PxFileBuf;
}
}

namespace nvidia
{
namespace apex
{
class ApexSDK;
class ModuleDestructible;
class DestructibleAsset;
}
using namespace physx::general_PxIOStream2;
}


namespace Nv
{
namespace Blast
{

struct CollisionHull;
class TkFramework;

namespace ApexImporter
{

struct ApexImporterConfig
{
	/**
		Interface search mode:

		EXACT -				-   Importer tries to find triangles from two chunks which lay in common surface.
								If such triangles are found, their intersections are considered as the interface.

		FORCED				-	Bond creation is forced no matter how far chunks from each other.

	*/
	enum InterfaceSearchMode { EXACT, FORCED, MODE_COUNT };

	ApexImporterConfig()
	{
		setDefaults();
	}

	void setDefaults()
	{
		infSearchMode				=	EXACT;
	}
	InterfaceSearchMode infSearchMode;
};


class ApexDestruction;


/** 
	ApexImportTool provides routines to create NvBlastAssets from APEX assets.
*/
class ApexImportTool
{
public:

	/**	
		Constructor should be provided with user defined allocator and massage function:
		\param[in] logFn		User - supplied message function(see NvBlastLog definition).May be NULL.
	*/
	ApexImportTool(NvBlastLog logFn = NULL);
	~ApexImportTool();

	//////////////////////////////////////////////////////////////////////////////

	/**
		Before using ApexImportTool should be initialized. ApexSDK and ModuleDestructible initialized internally.
		\return		If true, ApexImportTool initialized successfully.
	*/
	bool								initialize();

	/**
		Before using ApexImportTool should be initialized. User can provide existing ApexSDK and ModuleDestructible objects
		\param[in]	apexSdk					Pointer on ApexSDK object
		\param[in]	moduleDestructible		Pointer on ModuleDestructible object
		\return								If true, ApexImportTool initialized successfully.
	*/
	bool								initialize(nvidia::apex::ApexSDK* apexSdk, nvidia::apex::ModuleDestructible* moduleDestructible);

	/**
		Checks whether ApexImportTool is initialized and can be used.
		\return			If true, ApexImportTool initialized successfully.
	*/
	bool								isValid();


	/**
		Method loads APEX Destruction asset from file
		\param[in] stream		Pointer on PxFileBuf stream with Apex Destruction asset
		\return					If not 0, pointer on DestructibleAsset object is returned. 
	*/
	nvidia::apex::DestructibleAsset*	loadAssetFromFile(nvidia::PxFileBuf* stream);


	/**
		Method builds NvBlastAsset form provided DestructibleAsset. DestructibleAsset must contain support graph!
		\param[out] chunkReorderInvMap	Chunk map from blast chunk to apex chunk to be filled.
		\param[in] apexAsset			Pointer on DestructibleAsset object which should be converted to NvBlastAsset
		\param[out] chunkDescriptors	Reference on chunk descriptors array to be filled. 
		\param[out] bondDescriptors		Reference on bond descriptors array to be filled.
		\param[out] flags				Reference on chunk flags to be filled.
		
		\return							If true, output arrays are filled.
	*/
	bool								importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
											std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondDescriptors, std::vector<uint32_t>& flags);

	/**
		Method builds NvBlastAsset form provided DestructibleAsset. DestructibleAsset must contain support graph!
		Parameteres of conversion could be provided with ApexImporterConfig. 
		\param[out] chunkReorderInvMap	Chunk map from blast chunk to apex chunk to be filled.
		\param[in] apexAsset			Pointer on DestructibleAsset object which should be converted to NvBlastAsset
		\param[out] chunkDescriptors	Reference on chunk descriptors array to be filled.
		\param[out] bondDescriptors		Reference on bond descriptors array to be filled.
		\param[out] flags				Reference on chunk flags to be filled.
		\param[in] config				ApexImporterConfig object with conversion parameters, see above.
		\return							If true, output arrays are filled.
	*/
	bool								importApexAsset(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
											std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondDescriptors, std::vector<uint32_t>& flags,
											const ApexImporterConfig& config);


	/**
		Method serializes user-supplied NvBlastAsset object to user-supplied PxFileBuf stream.
		\param[in] asset		Pointer on NvBlastAsset object which should be serialized
		\param[in] stream		Pointer on PxFileBuf object in which NvBlastAsset should be serialized.
		\return					If true, NvBlastAsset object serialized successfully.
	*/
	bool								saveAsset(const NvBlastAsset* asset, nvidia::PxFileBuf* stream);

	/**
		Method creates collision geometry from user-supplied APEX Destructible asset.
		\param[in]	apexAsset				Pointer on DestructibleAsset object for which collision geometry should be created.
		\param[in]	chunkCount				Blast asset chunk count, should be equal to number of blast chunk descriptors which are gathered at ApexImportTool::importApexAsset(...)
		\param[in]	chunkReorderInvMap		Chunk map from blast chunk to apex chunk filled in ApexImportTool::importApexAsset(...)
		\param[in]	apexChunkFlags			Chunk flags array
		\param[out]	physicsChunks			Chunk physics info output array
		\param[out]	physicsSubchunks		Chunk collision geometry and transformation data output array
		\return								If true - success, output arrays are filled.
	*/
	bool								getCollisionGeometry(const nvidia::apex::DestructibleAsset* apexAsset, uint32_t chunkCount, std::vector<uint32_t>& chunkReorderInvMap,
												const std::vector<uint32_t>& apexChunkFlags, std::vector<ExtPxAssetDesc::ChunkDesc>& physicsChunks,
												std::vector<ExtPxAssetDesc::SubchunkDesc>& physicsSubchunks);

	ApexDestruction*					m_apexDestruction;
	//////////////////////////////////////////////////////////////////////////////

private:
	bool								importApexAssetInternal(std::vector<uint32_t>& chunkReorderInvMap, const nvidia::apex::DestructibleAsset* apexAsset,
											std::vector<NvBlastChunkDesc>& chunkDescriptors, std::vector<NvBlastBondDesc>& bondDesc, std::vector<uint32_t>& flags,
											const ApexImporterConfig& configDesc);
	

protected:
	NvBlastLog	m_log;

protected:
	ApexImportTool(const ApexImportTool&);
	ApexImportTool& operator=(const ApexImportTool&);
};

} // namespace ApexImporter

} // namespace Blast
} // namespace Nv

#endif // NVBLASTEXTAPEXIMPORTTOOL_H
