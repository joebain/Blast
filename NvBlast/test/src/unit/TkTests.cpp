#include "TkBaseTest.h"

#include <map>
#include <random>
#include <algorithm>

#include "PsMemoryBuffer.h"

#include "NvBlastTkSerializable.h"

#include "NvBlastTime.h"


struct ExpectedVisibleChunks 
{
	ExpectedVisibleChunks() :numActors(0), numChunks(0) {}
	ExpectedVisibleChunks(size_t a, size_t c) :numActors(a), numChunks(c) {}
	size_t numActors; size_t numChunks;
};

void testResults(std::vector<TkFamily*>& families, std::map<TkFamily*, ExpectedVisibleChunks>& expectedVisibleChunks)
{
	size_t numActors = 0;
	for (TkFamily* fam : families)
	{
		auto ex = expectedVisibleChunks[fam];
		EXPECT_EQ(ex.numActors, fam->getActorCount());
		numActors += ex.numActors;
		std::vector<TkActor*> actors(fam->getActorCount());
		fam->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		for (TkActor* actor : actors)
		{
			EXPECT_EQ(ex.numChunks, actor->getVisibleChunkCount());
		}
	}

	size_t numActorsExpected = 0;
	for (auto expected : expectedVisibleChunks)
	{
		numActorsExpected += expected.second.numActors;
	}

	EXPECT_EQ(numActorsExpected, numActors);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//													Tests
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


TEST_F(TkTestStrict, CreateFramework)
{
	createFramework();
	releaseFramework();
}

TEST_F(TkTestStrict, CreateAsset)
{
	createFramework();

	createTestAssets();
	releaseTestAssets();

	releaseFramework();
}

#if USE_PHYSX_DISPATCHER
TEST_F(TkTestStrict, DISABLED_MemLeak)
{
	PxFoundation* pxFoundation = PxCreateFoundation(PX_FOUNDATION_VERSION, *this, *this);
	PxU32 affinity[] = { 1, 2, 4, 8 };
	PxDefaultCpuDispatcher* cpuDispatcher = PxDefaultCpuDispatcherCreate(4, affinity);
	cpuDispatcher->setRunProfiled(false);
	PxTaskManager* taskman = PxTaskManager::createTaskManager(*this, cpuDispatcher, nullptr);

	cpuDispatcher->release();
	taskman->release();
	pxFoundation->release();
}
#endif

TEST_F(TkTestAllowWarnings, ActorDamageNoGroup)
{
	createFramework();
	createTestAssets();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	TkActorDesc actorDesc;
	actorDesc.asset = testAssets[0];
	TkActor* actor = fwk->createActor(actorDesc);

	const size_t bondFractureCount = 4;
	NvBlastFractureBuffers commands;
	NvBlastBondFractureData bdata[bondFractureCount];
	for (uint32_t i = 0; i < bondFractureCount; i++)
	{
		bdata[i].nodeIndex0 = 2 * i + 0;
		bdata[i].nodeIndex1 = 2 * i + 1;
		bdata[i].health = 1.0f;
	}
	commands.bondFractureCount = bondFractureCount;
	commands.bondFractures = bdata;
	commands.chunkFractureCount = 0;
	commands.chunkFractures = nullptr;
	actor->applyFracture(&commands, &commands);

	TkFamily& family = actor->getFamily();

	EXPECT_TRUE(commands.bondFractureCount == 4);
	EXPECT_TRUE(actor->isPending());

	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group = fwk->createGroup(gdesc);
	EXPECT_TRUE(group != nullptr);

	group->addActor(*actor);

	group->process();
	group->sync(true);

	EXPECT_FALSE(actor->isPending());
	EXPECT_EQ(2, family.getActorCount());

	releaseFramework();
}

TEST_F(TkTestAllowWarnings, ActorDamageGroup)
{
	TEST_ZONE_BEGIN("ActorDamageGroup");

	createFramework();
	createTestAssets();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	TestFamilyTracker ftrack1, ftrack2;

	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group = fwk->createGroup(gdesc);
	EXPECT_TRUE(group != nullptr);

	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	NvBlastExtShearDamageDesc shearDamage = getShearDamageDesc(0, 0, 0);

	std::vector<TkFamily*> families;
	TkFamily* trackedFamily;
	std::map<TkFamily*, ExpectedVisibleChunks> expectedVisibleChunks;

	{
		TkActorDesc adesc(testAssets[0]);

		TkActor* actor1 = fwk->createActor(adesc);
		EXPECT_TRUE(actor1 != nullptr);

		TkActor* actor2 = fwk->createActor(adesc);
		EXPECT_TRUE(actor2 != nullptr);


		expectedVisibleChunks[&actor1->getFamily()] = ExpectedVisibleChunks(8, 1); // full damage
		expectedVisibleChunks[&actor2->getFamily()] = ExpectedVisibleChunks(1, 1); // not split

		GeneratorAsset cube;
		generateCube(cube, 5, 2);
		TkAssetDesc assetDesc;
		assetDesc.bondCount = (uint32_t)cube.solverBonds.size();
		assetDesc.bondDescs = cube.solverBonds.data();
		assetDesc.chunkCount = (uint32_t)cube.chunks.size();
		assetDesc.chunkDescs = cube.solverChunks.data();
		assetDesc.bondFlags = nullptr;

		TkAsset* cubeAsset = fwk->createAsset(assetDesc);
		testAssets.push_back(cubeAsset);

		TkActorDesc cubeAD(cubeAsset);

		TkActor* cubeActor1 = fwk->createActor(cubeAD);
		EXPECT_TRUE(cubeActor1 != nullptr);

		trackedFamily = &cubeActor1->getFamily();
		cubeActor1->getFamily().addListener(ftrack1);

		TkActor* cubeActor2 = fwk->createActor(cubeAD);
		EXPECT_TRUE(cubeActor2 != nullptr);

		CSParams p(0, 0.0f);

		expectedVisibleChunks[&cubeActor1->getFamily()] = ExpectedVisibleChunks(2, 4); // split in 2, 4 chunks each
		expectedVisibleChunks[&cubeActor2->getFamily()] = ExpectedVisibleChunks(1, 1); // not split

		ftrack1.insertActor(cubeActor1);
		ftrack2.insertActor(actor1);

		actor1->getFamily().addListener(ftrack2);

		TEST_ZONE_BEGIN("add to groups");
		group->addActor(*cubeActor1);
		group->addActor(*cubeActor2);
		group->addActor(*actor1);
		group->addActor(*actor2);
		TEST_ZONE_END("add to groups");

		families.push_back(&cubeActor1->getFamily());
		families.push_back(&cubeActor2->getFamily());
		families.push_back(&actor1->getFamily());
		families.push_back(&actor2->getFamily());

		cubeActor1->damage(getCubeSlicerProgram(), &p, sizeof(p), getDefaultMaterial());
		actor1->damage(getFalloffProgram(), &radialDamage, sizeof(radialDamage), getDefaultMaterial());
	}

	EXPECT_FALSE(group->sync(true));
	EXPECT_FALSE(group->sync(false));

	group->process();
	group->sync();

	testResults(families, expectedVisibleChunks);


	{
		std::vector<TkActor*> actors(trackedFamily->getActorCount());
		trackedFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		for (TkActor* actor : actors)
		{
			CSParams p(1, 0.0f);
			actor->damage(getCubeSlicerProgram(), &p, sizeof(p), getDefaultMaterial());
		}
	}
	expectedVisibleChunks[trackedFamily] = ExpectedVisibleChunks(4, 2);

	group->process();
	group->sync();

	testResults(families, expectedVisibleChunks);


	{
		std::vector<TkActor*> actors(trackedFamily->getActorCount());
		trackedFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		for (TkActor* actor : actors)
		{
			CSParams p(2, 0.0f);
			actor->damage(getCubeSlicerProgram(), &p, sizeof(p), getDefaultMaterial());
		}
	}

	expectedVisibleChunks[trackedFamily] = ExpectedVisibleChunks(8, 1);

	group->process();
	group->sync();

	testResults(families, expectedVisibleChunks);


	{
		std::vector<TkActor*> actors(trackedFamily->getActorCount());
		trackedFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		TEST_ZONE_BEGIN("damage");
		for (TkActor* actor : actors)
		{
			actor->damage(getFalloffProgram(), &radialDamage, sizeof(radialDamage), getDefaultMaterial());
		}
		TEST_ZONE_END("damage");
	}
	expectedVisibleChunks[trackedFamily] = ExpectedVisibleChunks(4096, 1);

	group->process();
	while (!group->sync(true));

	testResults(families, expectedVisibleChunks);



	{
		std::vector<TkActor*> actors(trackedFamily->getActorCount());
		trackedFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		TEST_ZONE_BEGIN("damage");
		for (TkActor* actor : actors)
		{
			actor->damage(getShearProgram(), &shearDamage, sizeof(shearDamage), getDefaultMaterial());
		}
		TEST_ZONE_END("damage");
	}

	group->process();
	while (!group->sync(true))
		;



	{
		std::vector<TkActor*> actors(trackedFamily->getActorCount());
		trackedFamily->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		TEST_ZONE_BEGIN("damage");
		for (TkActor* actor : actors)
		{
			actor->damage(getShearProgram(), &shearDamage, sizeof(shearDamage), getDefaultMaterial());
		}
		TEST_ZONE_END("damage");
	}

	group->process();
	while (!group->sync(true));

	group->release();

	TEST_ZONE_BEGIN("family release");
	trackedFamily->release();
	TEST_ZONE_END("family release");

	releaseTestAssets();
	releaseFramework();

	TEST_ZONE_END("ActorDamageGroup");
}


TEST_F(TkTestAllowWarnings, ActorDamageMultiGroup)
{
	createFramework();
	createTestAssets();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	TestFamilyTracker ftrack1, ftrack2;

	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group0 = fwk->createGroup(gdesc);
	EXPECT_TRUE(group0 != nullptr);
	TkGroup* group1 = fwk->createGroup(gdesc);
	EXPECT_TRUE(group1 != nullptr);

	std::vector<TkFamily*> families(2);
	std::map<TkFamily*, ExpectedVisibleChunks> expectedVisibleChunks;

	// prepare 2 equal actors/families and damage
	{
		GeneratorAsset cube;
		generateCube(cube, 6, 2, 5);
		TkAssetDesc assetDesc;
		assetDesc.bondCount = (uint32_t)cube.solverBonds.size();
		assetDesc.bondDescs = cube.solverBonds.data();
		assetDesc.chunkCount = (uint32_t)cube.chunks.size();
		assetDesc.chunkDescs = cube.solverChunks.data();
		assetDesc.bondFlags = nullptr;

		TkAsset* cubeAsset = fwk->createAsset(assetDesc);
		testAssets.push_back(cubeAsset);

		TkActorDesc cubeAD(cubeAsset);

		TkActor* cubeActor0 = fwk->createActor(cubeAD);
		EXPECT_TRUE(cubeActor0 != nullptr);
		cubeActor0->getFamily().addListener(ftrack1);

		TkActor* cubeActor1 = fwk->createActor(cubeAD);
		EXPECT_TRUE(cubeActor1 != nullptr);
		cubeActor1->getFamily().addListener(ftrack2);

		ftrack1.insertActor(cubeActor0);
		ftrack2.insertActor(cubeActor1);

		group0->addActor(*cubeActor0);
		group1->addActor(*cubeActor1);

		families[0] = (&cubeActor0->getFamily());
		families[1] = (&cubeActor1->getFamily());

		{
			CSParams p0(0, 0.0f);
			CSParams p1(1, 0.0f);
			cubeActor0->damage(getCubeSlicerProgram(), &p0, sizeof(p0), getDefaultMaterial());
			cubeActor0->damage(getCubeSlicerProgram(), &p1, sizeof(p1), getDefaultMaterial());

			cubeActor1->damage(getCubeSlicerProgram(), &p0, sizeof(p0), getDefaultMaterial());
		}

		expectedVisibleChunks[families[0]] = ExpectedVisibleChunks(4, 2); // split in 4, 2 chunks each
		expectedVisibleChunks[families[1]] = ExpectedVisibleChunks(2, 4); // split in 2, 4 chunks each
	}

	// async process 2 groups
	{
		EXPECT_TRUE(group0->process());
		EXPECT_TRUE(group1->process());
		uint32_t completed = 0;
		while (completed < 2)
		{
			if (group0->sync(false))
				completed++;
			if (group1->sync(false))
				completed++;
		}
	}
	
	// checks
	testResults(families, expectedVisibleChunks);
	EXPECT_EQ(families[0]->getActorCount(), 4);
	EXPECT_EQ(group0->getActorCount(), 4);
	EXPECT_EQ(families[1]->getActorCount(), 2);
	EXPECT_EQ(group1->getActorCount(), 2);

	// we have group0 with 4 actors 2 chunks:
	// group0: [2]'  [2]'  [2]'  [2]' (family0')
	// group1: [4]'' [4]''            (family1'')
	// rearrange:
	// group0: [2]'  [2]'  [4]''
	// group1: [4]'' [2]'  [2]'
	{
		TkActor* group0Actors[2];
		group0->getActors(group0Actors, 2, 1); // start index: 1, because..why not?
		TkActor* group1Actors[2];
		group1->getActors(group1Actors, 2, 0);
		group0Actors[0]->removeFromGroup();
		group1->addActor(*group0Actors[0]);
		group0Actors[1]->removeFromGroup();
		group1->addActor(*group0Actors[1]);
		group1Actors[0]->removeFromGroup();
		group0->addActor(*group1Actors[0]);
	}

	// checks
	EXPECT_EQ(families[0]->getActorCount(), 4);
	EXPECT_EQ(group0->getActorCount(), 3);
	EXPECT_EQ(families[1]->getActorCount(), 2);
	EXPECT_EQ(group1->getActorCount(), 3);

	// damage all
	{
		TkActor* allActors[6];
		families[0]->getActors(allActors, 4, 0);
		families[1]->getActors(allActors + 4, 2, 0);

		typedef std::pair<TkGroup*, TkFamily*> pair;
		std::set<pair> combinations;
		for (auto actor : allActors)
		{
			combinations.emplace(pair(actor->getGroup(), &actor->getFamily()));
			if (actor->getVisibleChunkCount() == 4)
			{
				CSParams p1(1, 0.0f);
				actor->damage(getCubeSlicerProgram(), &p1, sizeof(p1), getDefaultMaterial());
			}
			CSParams p2(2, 0.0f);
			actor->damage(getCubeSlicerProgram(), &p2, sizeof(p2), getDefaultMaterial());
		}
		EXPECT_EQ(combinations.size(), 4);

		expectedVisibleChunks[families[0]] = ExpectedVisibleChunks(8, 1); // split in 8, 1 chunks each
		expectedVisibleChunks[families[1]] = ExpectedVisibleChunks(8, 1); // split in 8, 1 chunks each
	}

	// async process 2 groups
	{
		EXPECT_TRUE(group1->process());
		EXPECT_TRUE(group0->process());
		uint32_t completed = 0;
		while (completed < 2)
		{
			if (group0->sync(false))
				completed++;
			if (group1->sync(false))
				completed++;
		}
	}

	// checks
	testResults(families, expectedVisibleChunks);
	EXPECT_EQ(families[0]->getActorCount(), 8);
	EXPECT_EQ(ftrack1.actors.size(), 8);
	EXPECT_EQ(group0->getActorCount(), 8);
	EXPECT_EQ(families[1]->getActorCount(), 8);
	EXPECT_EQ(ftrack2.actors.size(), 8);
	EXPECT_EQ(group1->getActorCount(), 8);

	// damage till the end, aggressively
	std::default_random_engine re;
	{
		NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
		NvBlastExtShearDamageDesc shearDamage = getShearDamageDesc(0, 0, 0);

		std::vector<TkActor*> actors;
		while (1)
		{
			TEST_ZONE_BEGIN("damage loop");
			uint32_t n0 = families[0]->getActorCount();
			uint32_t n1 = families[1]->getActorCount();
			actors.resize(n0 + n1);
			families[0]->getActors(actors.data(), n0, 0);
			families[1]->getActors(actors.data() + n0, n1, 0);

			bool workTBD = false;
			for (TkActor* actor : actors)
			{
				if (!NvBlastActorCanFracture(actor->getActorLL(), nullptr))
				{
					continue;
				}

				workTBD = true;

				if (actor->getGraphNodeCount() > 1)
				{
					actor->damage(getFalloffProgram(), &radialDamage, sizeof(radialDamage), getDefaultMaterial());
				}
				else
				{
					actor->damage(getShearProgram(), &shearDamage, sizeof(shearDamage), getDefaultMaterial());
				}

				if (re() % 1000 < 500)
				{
					// switch group
					TkGroup* newGroup = actor->getGroup() == group0 ? group1 : group0;
					actor->removeFromGroup();
					newGroup->addActor(*actor);
				}
			}

			if (!workTBD)
				break;

			// async process 2 groups
			{
				EXPECT_TRUE(group1->process());
				EXPECT_TRUE(group0->process());
				uint32_t completed = 0;
				while (completed < 2)
				{
					if (group0->sync(false))
						completed++;
					if (group1->sync(false))
						completed++;
				}
			}
			TEST_ZONE_END("damage loop");
		}
	}

	// checks
	EXPECT_EQ(families[0]->getActorCount(), ftrack1.actors.size());
	EXPECT_EQ(families[1]->getActorCount(), ftrack2.actors.size());
	EXPECT_EQ(65536, families[0]->getActorCount() + families[1]->getActorCount());
	EXPECT_EQ(65536, group0->getActorCount() + group1->getActorCount());

	group0->release();
	group1->release();

	for (auto f : families)
		f->release();

	releaseTestAssets();
	releaseFramework();
}

TEST_F(TkTestAllowWarnings, ActorDamageBufferedDamage)
{
	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();

	// group
	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group = fwk->createGroup(gdesc);
	EXPECT_TRUE(group != nullptr);
	
	// random engine
	std::default_random_engine re;

	// cube asset
	GeneratorAsset cube;
	generateCube(cube, 4, 2, 3);
	TkAssetDesc assetDesc;
	assetDesc.bondCount = (uint32_t)cube.solverBonds.size();
	assetDesc.bondDescs = cube.solverBonds.data();
	assetDesc.chunkCount = (uint32_t)cube.chunks.size();
	assetDesc.chunkDescs = cube.solverChunks.data();
	assetDesc.bondFlags = nullptr;
	TkAsset* cubeAsset = fwk->createAsset(assetDesc);
	testAssets.push_back(cubeAsset);

	// actor desc
	TkActorDesc cubeAD(cubeAsset);

	// test will be repated 'trials' times. Because of random shuffle inside.
	const uint32_t trials = 100;
	for (uint32_t i = 0; i < trials; i++)
	{
		// create actor
		TkActor* actor = fwk->createActor(cubeAD);
		EXPECT_TRUE(actor != nullptr);
		TkFamily* family = (&actor->getFamily());
		group->addActor(*actor);

		// damage 3 times with CubeSlicer 2 * 2 * 2 = 8 actors
		// damage 4 corners with falloff radial 4 * 2 = 8 actors
		// total 16 actors
		uint32_t expectedActorCount = 16;

		// fallof params
		const float P = 0.5f;
		const float R = 0.35f;

		// 2 of damage types would be through user's NvBlastDamageProgram, this pointer must live till group->sync()
		NvBlastExtRadialDamageDesc userR0 = getRadialDamageDesc(P, P, 0, R, R);
		NvBlastProgramParams userProgramParams0 =
		{
			&userR0,	// damageDescBuffer
			1,			// damageDescCount
			nullptr,	// material
		};

		NvBlastExtRadialDamageDesc userR1 = getRadialDamageDesc(-P, P, 0, R, R);
		NvBlastProgramParams userProgramParams1 =
		{
			&userR1,	// damageDescBuffer
			1,			// damageDescCount
			nullptr,	// material
		};

		// fill damage functions, shuffle and apply
		{
			CSParams p0(0, 0.0f);
			CSParams p1(1, 0.0f);
			CSParams p2(2, 0.0f);
			NvBlastExtRadialDamageDesc r0 = getRadialDamageDesc(P, -P, 0, R, R);
			NvBlastExtRadialDamageDesc r1 = getRadialDamageDesc(-P, -P, 0, R, R);

			const uint32_t damageCount = 7;
			std::vector<std::function<void(void)>> damageFns(damageCount);
			damageFns[0] = [&]() { actor->damage(getCubeSlicerProgram(), &p0, sizeof(p0), getDefaultMaterial()); };
			damageFns[1] = [&]() { actor->damage(getCubeSlicerProgram(), &p1, sizeof(p1), getDefaultMaterial()); };
			damageFns[2] = [&]() { actor->damage(getCubeSlicerProgram(), &p2, sizeof(p2), getDefaultMaterial()); };
			damageFns[3] = [&]() { actor->damage(getFalloffProgram(), &r0, sizeof(r0), getDefaultMaterial()); };
			damageFns[4] = [&]() { actor->damage(getFalloffProgram(), &r1, sizeof(r1), getDefaultMaterial()); };
			damageFns[5] = [&]() { actor->damage(getFalloffProgram(), &userProgramParams0); };
			damageFns[6] = [&]() { actor->damage(getFalloffProgram(), &userProgramParams1); };

			// shuffle order!
			std::shuffle(std::begin(damageFns), std::end(damageFns), re);

			for (uint32_t i = 0; i < damageCount; i++)
			{
				damageFns[i]();
			}
		}

		// sync
		EXPECT_TRUE(group->process());
		group->sync(true);

		const auto ac = family->getActorCount();

		// check
		EXPECT_EQ(family->getActorCount(), expectedActorCount);
		EXPECT_EQ(group->getActorCount(), expectedActorCount);

		// release
		std::vector<TkActor*> actors(family->getActorCount());
		family->getActors(actors.data(), static_cast<uint32_t>(actors.size()));
		for (auto a : actors)
			a->removeFromGroup();
		family->release();
	}

	group->release();
	releaseFramework();
}

TEST_F(TkTestStrict, CreateActor)
{
	createFramework();
	TkFramework* framework = NvBlastTkFrameworkGet();

	const uint32_t assetDescCount = sizeof(g_assetDescs) / sizeof(g_assetDescs[0]);

	std::vector<TkAsset*> assets(assetDescCount);

	// assets
	for (uint32_t i = 0; i < assetDescCount; ++i)
	{
		TkAssetDesc desc;
		reinterpret_cast<NvBlastAssetDesc&>(desc) = g_assetDescs[i];
		desc.bondFlags = nullptr;
		assets[i] = framework->createAsset(desc);
		EXPECT_TRUE(assets[i] != nullptr);
	}

	// actors
	std::vector<TkActor*> actors;;
	std::vector<TkFamily*> actorFamilies;;
	for (const TkAsset* asset : assets)
	{
		for (int i = 0; i < 2; i++)
		{
			TkActorDesc desc(asset);
			TkActor* actor = framework->createActor(desc);
			EXPECT_TRUE(actor != nullptr);
			EXPECT_TRUE(actor->getActorLL() != nullptr);
			//EXPECT_TRUE(&actor->getFamily() != nullptr);
			EXPECT_TRUE(actor->getFamily().getActorCount() == 1);
			actors.push_back(actor);
			EXPECT_TRUE(std::find(actorFamilies.begin(), actorFamilies.end(), &actor->getFamily()) == actorFamilies.end());
			actorFamilies.push_back(&actor->getFamily());

		}
	}

	// framework checks
	{
		std::vector<TkObject*> objects;

		// assets
		{
			const TkType* assetType = framework->getType(TkTypeIndex::Asset);
			objects.resize(framework->getObjectCount(*assetType));
			EXPECT_TRUE(framework->getObjects(reinterpret_cast<TkIdentifiable**>(objects.data()), static_cast<uint32_t>(objects.size()), *assetType) == static_cast<uint32_t>(objects.size()));
			ExpectArrayMatch(objects.data(), objects.size(), (TkObject**)assets.data(), assets.size());
		}

		// actors
#		if(0) // framework does not track actors explicitly anymore
		{
			const TkType* actorType = framework->getType(TkTypeIndex::Actor);
			objects.resize(framework->getObjectCount(*actorType));
			EXPECT_TRUE(framework->getObjects(reinterpret_cast<TkIdentifiable**>(objects.data()), objects.size(), *actorType) == objects.size());
			ExpectArrayMatch(objects.data(), objects.size(), (TkObject**)actors.data(), actors.size());
		}
#		endif
		// families
		{
			const TkType* familyType = framework->getType(TkTypeIndex::Family);
			objects.resize(framework->getObjectCount(*familyType));
			EXPECT_TRUE(framework->getObjects(reinterpret_cast<TkIdentifiable**>(objects.data()), static_cast<uint32_t>(objects.size()), *familyType) == static_cast<uint32_t>(objects.size()));
			ExpectArrayMatch(objects.data(), objects.size(), (TkObject**)actorFamilies.data(), actorFamilies.size());
		}
	}

	// release
	for (TkActor* actor : actors)
	{
		actor->release();
	}
	for (TkAsset* asset : assets)
	{
		asset->release();
	}

	releaseFramework();
}

template<int FailMask, int Verbosity>
TkFamily* TkBaseTest<FailMask, Verbosity>::familySerialization(TkFamily* family)
{
	TkFramework* fw = NvBlastTkFrameworkGet();

	const TkType* familyType = fw->getType(TkTypeIndex::Family);
	EXPECT_TRUE(familyType != nullptr);

	PsMemoryBuffer* membuf = PX_NEW(PsMemoryBuffer);
	EXPECT_TRUE(membuf != nullptr);
	if (membuf != nullptr)
	{
		const bool result = family->serialize(*membuf);
		EXPECT_EQ(true, result);
		if (!result)
		{
			return family;
		}
		const size_t familyActorCount = family->getActorCount();
		const TkAsset* familyAsset = family->getAsset();
		family->release();
		family = reinterpret_cast<TkFamily*>(fw->deserialize(*membuf));
		EXPECT_TRUE(family != nullptr);
		if (family != nullptr)
		{
			EXPECT_EQ(familyActorCount, family->getActorCount());
			EXPECT_EQ(familyAsset, family->getAsset());
		}
		membuf->release();
	}

	return family;
}

TEST_F(TkTestAllowWarnings, FamilySerialization)
{
	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();

	// group
	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group = fwk->createGroup(gdesc);
	EXPECT_TRUE(group != nullptr);

	// random engine
	std::default_random_engine re;

	// cube asset
	TkAsset* cubeAsset = createCubeAsset(4, 2, 3, false);

	// actor desc
	TkActorDesc cubeAD(cubeAsset);

	// create actor
	TkActor* actor = fwk->createActor(cubeAD);
	EXPECT_TRUE(actor != nullptr);
	TkFamily* family = (&actor->getFamily());

	// set an ID
	NvBlastID id;
	memcpy(id.data, "Observer-expectancy effect", sizeof(NvBlastID));	// Stuffing an arbitrary 16 bytes (The prefix of the given string)
	cubeAsset->setID(id);

	// serialize/deserialize 
	family = familySerialization(family);

	// fill damage functions, apply one by one and serialize family in between
	{
		// damage 3 times with CubeSlicer 2 * 2 * 2 = 8 actors
		// damage 4 corners with falloff radial 4 * 2 = 8 actors
		// total 16 actors
		uint32_t expectedActorCount = 16;

		// cube slicer params
		CSParams p0(0, 0.0f);
		CSParams p1(1, 0.0f);
		CSParams p2(2, 0.0f);

		// fallof params
		const float P = 0.5f;
		const float R = 0.35f;
		NvBlastExtRadialDamageDesc r0 = getRadialDamageDesc(P, P, 0, R, R);
		NvBlastExtRadialDamageDesc r1 = getRadialDamageDesc(-P, P, 0, R, R);
		NvBlastExtRadialDamageDesc r2 = getRadialDamageDesc(P, -P, 0, R, R);
		NvBlastExtRadialDamageDesc r3 = getRadialDamageDesc(-P, -P, 0, R, R);

		const uint32_t damageCount = 7;
		std::vector<std::function<void(TkActor* a)>> damageFns(damageCount);
		damageFns[0] = [&](TkActor* a) { a->damage(getCubeSlicerProgram(), &p0, sizeof(p0), getDefaultMaterial()); };
		damageFns[1] = [&](TkActor* a) { a->damage(getCubeSlicerProgram(), &p1, sizeof(p1), getDefaultMaterial()); };
		damageFns[2] = [&](TkActor* a) { a->damage(getCubeSlicerProgram(), &p2, sizeof(p2), getDefaultMaterial()); };
		damageFns[3] = [&](TkActor* a) { a->damage(getFalloffProgram(), &r0, sizeof(r0), getDefaultMaterial()); };
		damageFns[4] = [&](TkActor* a) { a->damage(getFalloffProgram(), &r1, sizeof(r1), getDefaultMaterial()); };
		damageFns[5] = [&](TkActor* a) { a->damage(getFalloffProgram(), &r2, sizeof(r2), getDefaultMaterial()); };
		damageFns[6] = [&](TkActor* a) { a->damage(getFalloffProgram(), &r3, sizeof(r3), getDefaultMaterial()); };

		std::vector<TkActor*> actors(64);

		for (uint32_t i = 0; i < damageCount; i++)
		{
			actors.resize(family->getActorCount());
			family->getActors(actors.data(), static_cast<uint32_t>(actors.size()));

			// damage
			for (auto actor : actors)
			{
				group->addActor(*actor);
				damageFns[i](actor);
			}

			// sync
			EXPECT_TRUE(group->process());
			group->sync(true);

			family = familySerialization(family);
		}

		// check
		EXPECT_EQ(family->getActorCount(), expectedActorCount);
	}

	// release
	family->release();

	group->release();
	releaseFramework();
}

TEST_F(TkTestStrict, GroupStats)
{
	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();

	// group
	TkGroupDesc gdesc;
	gdesc.pxTaskManager = m_taskman;
	TkGroup* group = fwk->createGroup(gdesc);
	EXPECT_TRUE(group != nullptr);

	TkAsset* cubeAsset = createCubeAsset(4, 2);
	TkActorDesc cubeDesc(cubeAsset);

	TkActor* cubeActor1 = fwk->createActor(cubeDesc);
	TkActor* cubeActor2 = fwk->createActor(cubeDesc);
	TkActor* cubeActor3 = fwk->createActor(cubeDesc);
	TkActor* cubeActor4 = fwk->createActor(cubeDesc);

	group->addActor(*cubeActor1);
	group->addActor(*cubeActor2);
	group->addActor(*cubeActor3);
	group->addActor(*cubeActor4);

	NvBlastExtRadialDamageDesc r0 = getRadialDamageDesc(0.0f, 0.0f, 0.0f);
	cubeActor1->damage(getFalloffProgram(), &r0, sizeof(r0));
	cubeActor2->damage(getFalloffProgram(), &r0, sizeof(r0));
	cubeActor3->damage(getFalloffProgram(), &r0, sizeof(r0));
	cubeActor4->damage(getFalloffProgram(), &r0, sizeof(r0));

	Nv::Blast::Time time;
	group->process();
	group->sync(true);
	int64_t groupTime = time.getElapsedTicks();

	TkGroupStats gstats;
	group->getStats(gstats);

	int64_t total = gstats.timers.fracture + gstats.timers.island + gstats.timers.material + gstats.timers.partition + gstats.timers.visibility;

#if NV_PROFILE
	EXPECT_GT(total, 0);									// some values are reported
	EXPECT_LT(groupTime, total);							// total LL time is higher than group time
	EXPECT_GT((double)gstats.workerTime / groupTime, 2.0);	// expect some minimal speedup (including overhead) 
	EXPECT_EQ(4, gstats.processedActorsCount);				// actors processed
#endif

	releaseFramework();
}

TEST_F(TkTestStrict, FractureReportSupport)
{
	createFramework();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	NvBlastChunkDesc chunkDescs[] =
	{
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::SupportFlag, 'prnt' },
		{ { -1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'left' },
		{ { +1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'rght' },
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 0;
	assetDesc.bondDescs = nullptr;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* actor = fwk->createActor(actorDesc);
	actor->userData = (void*)'root';

	class Listener : public TkEventListener
	{
		void receive(const TkEvent* events, uint32_t eventCount) override
		{
			for (uint32_t i = 0; i < eventCount; i++)
			{
				const TkEvent& event = events[i];
				switch (event.type)
				{
				case TkJointUpdateEvent::EVENT_TYPE:
					FAIL() << "not expecting joints here";
					break;

				case TkFractureCommands::EVENT_TYPE:
				{
					const TkActorData& actor = event.getPayload<TkFractureCommands>()->tkActorData;

					// Group::sync still needed the family for SharedMemory management.
					EXPECT_TRUE(nullptr != actor.family);

					EXPECT_EQ((void*)'root', actor.userData);
					EXPECT_EQ(0, actor.index);
				}
				break;

				case TkFractureEvents::EVENT_TYPE:
				{
					const TkActorData& actor = event.getPayload<TkFractureEvents>()->tkActorData;
					EXPECT_EQ((void*)'root', actor.userData);
					EXPECT_EQ(0, actor.index);
				}
				break;

				case TkSplitEvent::EVENT_TYPE:
				{
					const TkSplitEvent* split = event.getPayload<TkSplitEvent>();

					EXPECT_TRUE(nullptr != split->parentData.family);
					EXPECT_EQ((void*)'root', split->parentData.userData);
					EXPECT_EQ(0, split->parentData.index);

					EXPECT_EQ(2, split->numChildren);
					EXPECT_EQ(1, split->children[0]->getVisibleChunkCount());

					uint32_t visibleChunkIndex;
					// child order is not mandatory
					{
						TkActor* a = split->children[0];
						a->getVisibleChunkIndices(&visibleChunkIndex, 1);
						uint32_t li = a->getIndex();
						EXPECT_EQ(1, li);
						EXPECT_EQ(split->parentData.family, &a->getFamily());
						EXPECT_EQ('left', a->getAsset()->getChunks()[visibleChunkIndex].userData);
					}

					{
						TkActor*a = split->children[1];
						a->getVisibleChunkIndices(&visibleChunkIndex, 1);
						uint32_t ri = a->getIndex();
						EXPECT_EQ(2, ri);
						EXPECT_EQ(split->parentData.family, &a->getFamily());
						EXPECT_EQ('rght', a->getAsset()->getChunks()[visibleChunkIndex].userData);
					}
				}
				break;

				default:
					FAIL() << "should not get here";
				}
			}
		}
	} listener;
	actor->getFamily().addListener(listener);

	// expected state for the original actor, see Listener
	EXPECT_EQ((void*)'root', actor->userData);
	EXPECT_EQ(0, actor->getIndex());

	TkGroupDesc groupDesc = { m_taskman };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*actor);

	// this will trigger hierarchical chunk fracture
	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	actor->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	group->process();
	group->sync();

	releaseFramework();
}

TEST_F(TkTestStrict, FractureReportGraph)
{
	createFramework();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	NvBlastBond bondToBreak = { { 1,0,0 }, 1,{ 0, 0, 0 }, 0 };
	NvBlastBond bondToKeep = { { 1,0,0 }, 1,{ 10, 10, 10 }, 0 };
	NvBlastBondDesc bondDescs[] =
	{
		{ { 1,2 }, bondToKeep },
		{ { 2,3 }, bondToBreak },
	};

	NvBlastChunkDesc chunkDescs[] =
	{
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::NoFlags, 'root' },
		{ { -1,0,0 }, 1, 0, NvBlastChunkDesc::SupportFlag, 'A' },
		{ { +1,0,0 }, 1, 0, NvBlastChunkDesc::SupportFlag, 'B' },
		{ { +1,0,0 }, 1, 0, NvBlastChunkDesc::SupportFlag, 'C' },
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 2;
	assetDesc.bondDescs = bondDescs;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* rootActor = fwk->createActor(actorDesc);
	rootActor->userData = (void*)'root';

	class Listener : public TkEventListener
	{
		void receive(const TkEvent* events, uint32_t eventCount) override
		{
			for (uint32_t i = 0; i < eventCount; i++)
			{
				const TkEvent& event = events[i];
				switch (event.type)
				{
				case TkJointUpdateEvent::EVENT_TYPE:
					FAIL() << "not expecting joints here";
					break;

				case TkFractureCommands::EVENT_TYPE:
				{
					const TkActorData& actor = event.getPayload<TkFractureCommands>()->tkActorData;
					
					// Group::sync still needed the family for SharedMemory management.
					EXPECT_TRUE(nullptr != actor.family);

					// original actor state is not preserved, the last test will fail
					EXPECT_EQ((void*)'root', actor.userData);
					EXPECT_EQ(0, actor.index);

					// this information was invalid anyway
					//EXPECT_EQ(1, actor->getVisibleChunkCount()) << "state not preserved";
				}
				break;

				case TkFractureEvents::EVENT_TYPE:
				{
					const TkActorData& actor = event.getPayload<TkFractureEvents>()->tkActorData;

					// Group::sync still needed the family for SharedMemory management.
					EXPECT_TRUE(nullptr != actor.family);

					// original actor state is not preserved, the last test will fail
					EXPECT_EQ((void*)'root', actor.userData);
					EXPECT_EQ(0, actor.index);

					// this information was invalid anyway
					//EXPECT_EQ(1, actor->getVisibleChunkCount()) << "state not preserved";
				}
				break;

				case TkSplitEvent::EVENT_TYPE:
				{
					const TkSplitEvent* split = event.getPayload<TkSplitEvent>();
					EXPECT_EQ((void*)'root', split->parentData.userData);
					EXPECT_EQ(0, split->parentData.index);
					EXPECT_EQ(2, split->numChildren);

					uint32_t visibleChunkIndex[2];
					// child order is not mandatory
					{
						TkActor* a = split->children[1];
						EXPECT_EQ(2, a->getVisibleChunkCount()); // chunks A and B
						a->getVisibleChunkIndices(visibleChunkIndex, 2);
						uint32_t actorIndex = a->getIndex();
						EXPECT_EQ(0, actorIndex); // same index as the original actor

						// visible chunk order is not mandatory
						EXPECT_EQ('B', a->getAsset()->getChunks()[visibleChunkIndex[0]].userData);
						EXPECT_EQ('A', a->getAsset()->getChunks()[visibleChunkIndex[1]].userData);
					}

					{
						TkActor* a = split->children[0];
						EXPECT_EQ(1, a->getVisibleChunkCount()); 
						a->getVisibleChunkIndices(visibleChunkIndex, 1);
						uint32_t actorIndex = a->getIndex();
						EXPECT_EQ(2, actorIndex);
						EXPECT_EQ('C', a->getAsset()->getChunks()[visibleChunkIndex[0]].userData);
					}
				}
				break;

				default:
					FAIL() << "should not get here";
				}
			}
		}
	} listener;
	rootActor->getFamily().addListener(listener);

	// expected state for the original actor, see Listener
	EXPECT_EQ((void*)'root', rootActor->userData);
	EXPECT_EQ(0, rootActor->getIndex());
	EXPECT_EQ(1, rootActor->getVisibleChunkCount());

	TkGroupDesc groupDesc = { m_taskman };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*rootActor);

	// this will trigger one bond to break
	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0, 0.5f, 0.5f);
	rootActor->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	group->process();
	group->sync();

	releaseFramework();
}

TEST_F(TkTestStrict, SplitWarning) // GWD-167
{
	createFramework();

	TkFramework* fwk = NvBlastTkFrameworkGet();

	NvBlastChunkDesc chunkDescs[] =
	{
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::SupportFlag, 'root' },
		{ { -1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'A' },
		{ { +1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'B' },
		{ { -1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'C' },
		{ { +1,0,0 }, 1, 0, NvBlastChunkDesc::NoFlags, 'D' },
		{ { -1,0,0 }, 1, 1, NvBlastChunkDesc::NoFlags, 'AAAA' },
		{ { +1,0,0 }, 1, 2, NvBlastChunkDesc::NoFlags, 'BBBB' },
		{ { -1,0,0 }, 1, 3, NvBlastChunkDesc::NoFlags, 'CCCC' },
		{ { +1,0,0 }, 1, 4, NvBlastChunkDesc::NoFlags, 'DDDD' },
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 0;
	assetDesc.bondDescs = nullptr;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* actor = fwk->createActor(actorDesc);

	TkGroupDesc groupDesc = { m_taskman };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*actor);

	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	actor->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	group->process();
	group->sync();

	releaseFramework();
}


TEST_F(TkTestAllowWarnings, ChangeThreadCountToZero)
{
	// tests that group still allocates memory for one worker 
	// by replacing to a 0 threads cpu dispatcher (warns)
	// mainly relies on internal asserts

	class EventCounter : public TkEventListener
	{
	public:
		EventCounter() :fracCommands(0), fracEvents(0) {}

		void receive(const TkEvent* events, uint32_t eventCount)
		{
			for (uint32_t i = 0; i < eventCount; i++)
			{
				const TkEvent& event = events[i];
				switch (event.type)
				{
				case TkFractureCommands::EVENT_TYPE:
					fracCommands++;
					break;
				case TkFractureEvents::EVENT_TYPE:
					fracEvents++;
					break;
				default:
					FAIL();
					// no split due to single chunk
					// no joints
				}
			}
		}

		uint32_t fracCommands, fracEvents;
	} listener;

	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();
	NvBlastChunkDesc chunkDescs[] = {
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::SupportFlag, 'root' }
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 0;
	assetDesc.bondDescs = nullptr;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* actor1 = fwk->createActor(actorDesc);
	TkActor* actor2 = fwk->createActor(actorDesc);
	TkActor* actor3 = fwk->createActor(actorDesc);
	TkActor* actor4 = fwk->createActor(actorDesc);

	actor1->getFamily().addListener(listener);
	actor2->getFamily().addListener(listener);
	actor3->getFamily().addListener(listener);
	actor4->getFamily().addListener(listener);

#if USE_PHYSX_DISPATCHER
	PxU32 affinity[] = { 1, 2, 4, 8 };
	PxDefaultCpuDispatcher* disp0 = PxDefaultCpuDispatcherCreate(0, affinity);
	disp0->setRunProfiled(false);
	PxDefaultCpuDispatcher* disp4 = PxDefaultCpuDispatcherCreate(4, affinity);
	disp4->setRunProfiled(false);
#else
	TestCpuDispatcher* disp0 = new TestCpuDispatcher(0);
	TestCpuDispatcher* disp4 = new TestCpuDispatcher(4);
#endif

	PxTaskManager* taskman = PxTaskManager::createTaskManager(*this, disp4);

	TkGroupDesc groupDesc = { taskman };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*actor1);
	group->addActor(*actor2);
	taskman->setCpuDispatcher(*disp0);
	group->addActor(*actor3);
	group->addActor(*actor4);

	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	actor1->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor2->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor3->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor4->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	group->process();
	group->sync();

	EXPECT_EQ(4, listener.fracCommands);
	EXPECT_EQ(4, listener.fracEvents);

	releaseFramework();

	disp0->release();
	disp4->release();
	taskman->release();
}

TEST_F(TkTestAllowWarnings, ChangeThreadCountUp)
{
	// tests that group allocates more memory for additional workers 
	// by replacing to a higher thread count cpu dispatcher (warns)
	// mainly relies on internal asserts

	class EventCounter : public TkEventListener
	{
	public:
		EventCounter() :fracCommands(0), fracEvents(0) {}

		void receive(const TkEvent* events, uint32_t eventCount)
		{
			for (uint32_t i = 0; i < eventCount; i++)
			{
				const TkEvent& event = events[i];
				switch (event.type)
				{
				case TkFractureCommands::EVENT_TYPE:
					fracCommands++;
					break;
				case TkFractureEvents::EVENT_TYPE:
					fracEvents++;
					break;
				default:
					FAIL();
					// no split due to single chunk
					// no joints
				}
			}
		}

		uint32_t fracCommands, fracEvents;
	} listener;

	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();
	NvBlastChunkDesc chunkDescs[] = {
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::SupportFlag, 'root' }
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 0;
	assetDesc.bondDescs = nullptr;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* actor1 = fwk->createActor(actorDesc);
	TkActor* actor2 = fwk->createActor(actorDesc);
	TkActor* actor3 = fwk->createActor(actorDesc);
	TkActor* actor4 = fwk->createActor(actorDesc);

	actor1->getFamily().addListener(listener);
	actor2->getFamily().addListener(listener);
	actor3->getFamily().addListener(listener);
	actor4->getFamily().addListener(listener);

#if USE_PHYSX_DISPATCHER
	PxU32 affinity[] = { 1, 2, 4, 8 };
	PxDefaultCpuDispatcher* disp2 = PxDefaultCpuDispatcherCreate(2, affinity);
	disp2->setRunProfiled(false);
	PxDefaultCpuDispatcher* disp4 = PxDefaultCpuDispatcherCreate(4, affinity);
	disp4->setRunProfiled(false);
#else
	TestCpuDispatcher* disp2 = new TestCpuDispatcher(2);
	TestCpuDispatcher* disp4 = new TestCpuDispatcher(4);
#endif

	PxTaskManager* taskman = PxTaskManager::createTaskManager(*this, disp2);

	TkGroupDesc groupDesc = { taskman };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*actor1);
	group->addActor(*actor2);
	group->addActor(*actor3);
	group->addActor(*actor4);

	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	actor1->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor2->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor3->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor4->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	taskman->setCpuDispatcher(*disp4);

	group->process();
	group->sync();

	EXPECT_EQ(4, listener.fracCommands);
	EXPECT_EQ(4, listener.fracEvents);

	releaseFramework();

	disp2->release();
	disp4->release();
	taskman->release();
}

TEST_F(TkTestAllowWarnings, GroupNoWorkers)
{
	// tests that group still works without a taskmanager
	// a warnings is expected
	// mainly relies on internal asserts

	class EventCounter : public TkEventListener
	{
	public:
		EventCounter() :fracCommands(0), fracEvents(0) {}

		void receive(const TkEvent* events, uint32_t eventCount)
		{
			for (uint32_t i = 0; i < eventCount; i++)
			{
				const TkEvent& event = events[i];
				switch (event.type)
				{
				case TkFractureCommands::EVENT_TYPE:
					fracCommands++;
					break;
				case TkFractureEvents::EVENT_TYPE:
					fracEvents++;
					break;
				default:
					FAIL();
					// no split due to single chunk
					// no joints
				}
			}
		}

		uint32_t fracCommands, fracEvents;
	} listener;

	createFramework();
	TkFramework* fwk = NvBlastTkFrameworkGet();
	NvBlastChunkDesc chunkDescs[] = {
		{ { 0,0,0 }, 2, UINT32_MAX, NvBlastChunkDesc::SupportFlag, 'root' }
	};

	TkAssetDesc assetDesc;
	assetDesc.chunkCount = sizeof(chunkDescs) / sizeof(NvBlastChunkDesc);
	assetDesc.chunkDescs = chunkDescs;
	assetDesc.bondCount = 0;
	assetDesc.bondDescs = nullptr;
	assetDesc.bondFlags = nullptr;
	const TkAsset* asset = fwk->createAsset(assetDesc);

	TkActorDesc actorDesc;
	actorDesc.asset = asset;
	TkActor* actor1 = fwk->createActor(actorDesc);
	TkActor* actor2 = fwk->createActor(actorDesc);
	TkActor* actor3 = fwk->createActor(actorDesc);
	TkActor* actor4 = fwk->createActor(actorDesc);

	actor1->getFamily().addListener(listener);
	actor2->getFamily().addListener(listener);
	actor3->getFamily().addListener(listener);
	actor4->getFamily().addListener(listener);

	TkGroupDesc groupDesc = { nullptr };
	TkGroup* group = fwk->createGroup(groupDesc);

	group->addActor(*actor1);
	group->addActor(*actor2);
	group->addActor(*actor3);
	group->addActor(*actor4);

	NvBlastExtRadialDamageDesc radialDamage = getRadialDamageDesc(0, 0, 0);
	actor1->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor2->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor3->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());
	actor4->damage(getFalloffProgram(), &radialDamage, sizeof(NvBlastExtRadialDamageDesc), getDefaultMaterial());

	group->process();
	group->sync();

	EXPECT_EQ(4, listener.fracCommands);
	EXPECT_EQ(4, listener.fracEvents);

	releaseFramework();
}

