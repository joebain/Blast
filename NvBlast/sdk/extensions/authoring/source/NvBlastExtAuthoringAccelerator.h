/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTAUTHORINGACCELERATOR_H
#define NVBLASTEXTAUTHORINGACCELERATOR_H

#include <set>
#include <vector>
#include "NvBlastExtAuthoringTypes.h"


namespace Nv
{
namespace Blast
{

class Mesh;


/**
	Acceleration structure interface.
*/
class SpatialAccelerator
{
public:
	/**
		Set state of accelerator to return all facets which possibly can intersect given facet.
		\param[in] pos Vertex buffer
		\param[in] ed Edge buffer
		\param[in] fc Facet which should be tested.
	*/
	virtual void	setState(Vertex* pos, Edge* ed, Facet& fc) = 0;
	/**
		Set state of accelerator to return all facets which possibly can cover given point. Needed for testing whether point is inside mesh.
		\param[in] point Point which should be tested.
	*/
	virtual void	setState(const physx::PxVec3& point) = 0;
	/**
		Recieve next facet for setted state.
		\return Next facet index, or -1 if no facets left.
	*/
	virtual int32_t	getNextFacet() = 0;

	virtual ~SpatialAccelerator() {};
};


/**
	Dummy accelerator iterates through all facets of mesh.
*/
class DummyAccelerator : public SpatialAccelerator
{
public:
	/**
		\param[in] count Mesh facets count for which accelerator should be built.
	*/
	DummyAccelerator(int32_t count);
	virtual void setState(Vertex* pos, Edge* ed, Facet& fc);
	virtual void setState(const physx::PxVec3& point);
	virtual int32_t getNextFacet();

private:
	int32_t count;
	int32_t current;
};

/**
	Accelerator which builds map from 3d grid to initial mesh facets. 
	To find all facets which possibly intersect given one, it return all facets which are pointed by grid cells, which intersects with bounding box of given facet.
	To find all facets which possibly cover given point, all facets which are pointed by cells in column which contains given point are returned.
*/
class BBoxBasedAccelerator : public SpatialAccelerator
{
public:
	/**
		\param[in] mesh Mesh for which acceleration structure should be built.
		\param[in] resolution Resolution on 3d grid.
	*/
	BBoxBasedAccelerator(Mesh* mesh, int32_t resolution);
	virtual ~BBoxBasedAccelerator();
	int32_t getNextFacet();
	void setState(Vertex* pos, Edge* ed, Facet& fc);
	void setState(const physx::PxVec3& p);
private:

	bool testCellPolygonIntersection(int32_t cellId, physx::PxBounds3& facetBB);
	void buildAccelStructure(Vertex* pos, Edge* edges, Facet* fc, int32_t facetCount);

	int32_t mResolution;
	physx::PxBounds3 mBounds;
	physx::PxBounds3 facetBox;
	std::vector< std::vector<int32_t> > mSpatialMap;
	std::vector<physx::PxBounds3> mCells;

	
	// Iterator data
	std::vector<uint32_t> alreadyGotFlag;
	uint32_t alreadyGotValue;
	std::vector<int32_t> cellList;
	int32_t mIteratorCell;
	int32_t mIteratorFacet;
};



/**
	Accelerator which builds map from 3d grid to initial mesh facets.
	To find all facets which possibly intersect given one, it return all facets which are pointed by grid cells, which are intersected by given facet.
	To find all facets which possibly cover given point, all facets which are pointed by cells in column which contains given point are returned.

	In difference with BBoxBasedAccelerator this accelerator computes actual intersection of cube with polygon. It is more precise and omits much more intersections but slower.
*/

class IntersectionTestingAccelerator : public SpatialAccelerator
{
public:
	IntersectionTestingAccelerator(Mesh* mesh, int32_t resolution);
	int32_t getNextFacet();
	void setState(Vertex* pos, Edge* ed, Facet& fc);
	void setState(const physx::PxVec3& p);


private:
	std::vector< std::vector<int32_t> > mSpatialMap;
	std::vector<physx::PxBounds3> mCubes;
	int32_t mResolution;

	// Iterator data
	std::vector<uint32_t> alreadyGotFlag;
	uint32_t alreadyGotValue;
	std::vector<int32_t> cellList;
	int32_t mIteratorCell;
	int32_t mIteratorFacet;
};

} // namespace Blast
} // namsepace Nv


#endif // ifndef NVBLASTEXTAUTHORINGACCELERATOR_H
