using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEditor;
using UnityEngine;

public class Breakable : MonoBehaviour
{
    //private NvBlastAssetDesc blastAssetDesc;
    [SerializeField]
    private NvBlastAsset blastAsset;
    private NvBlastFamily blastFamily;

    private Dictionary<Rigidbody, NvBlastActor> actors = new Dictionary<Rigidbody, NvBlastActor>();
    private BreakableChunk[] chunks;
    private NvBlastFractureBuffers fractureBuffers;
    private IntPtr newActorsBuffer;
    private uint leafChunkCount;

    public GameObject BlastAssetObj;
    public TextAsset BlastAsset;
    //string BlastAssetPath;

    int chunkCount, bondCount;

    void Start()
    {
        ImportBlastAsset();
    }

    public void ImportBlastAsset()
    {
        //if (BlastAssetPath == null || BlastAssetPath == "")
        //{
        //    BlastAssetPath = Application.dataPath + "/" + AssetDatabase.GetAssetPath(BlastAsset).Substring(7); // get rid of leading "Assets/"
        //}

        if (BlastAssetObj == null)
        {
            BlastAssetObj = gameObject;
        }

        //////////////////////////////////////
        //////// TRY TO LOAD A TK ASSET
        //////////////////////////////////////

        //NvBlastTkAsset tkAsset = NvBlastTkAsset.Deserialize(BlastAssetPath);
        NvBlastTkAsset tkAsset = NvBlastTkAsset.Deserialize(BlastAsset);

        Debug.Log("loaded tk asset");

        chunkCount = tkAsset.GetChunkCount();
        bondCount = tkAsset.GetBondCount();

        Debug.Log("chunk count: " + chunkCount);
        Debug.Log("bond count: " + bondCount);

        blastAsset = tkAsset.GetAssetLL();


        // make chunks from the imported obj
        chunks = new BreakableChunk[chunkCount];
        for (int c = 0; c < BlastAssetObj.transform.childCount; c++)
        {
            Transform chunkTransform = BlastAssetObj.transform.GetChild(c);
            BreakableChunk chunk = chunkTransform.GetComponent<BreakableChunk>();
            if (chunk == null)
            {
                chunk = chunkTransform.gameObject.AddComponent<BreakableChunk>();
            }
            if (chunkTransform.GetComponent<MeshCollider>() == null)
            {
                MeshCollider collider = chunkTransform.gameObject.AddComponent<MeshCollider>();
                collider.convex = true;
            }
            chunk.Initialize(tkAsset.GetChunk(c));
            chunks[c] = chunk;
            chunk.gameObject.SetActive(false);
        }

        // First actor
        blastFamily = new NvBlastFamily(blastAsset);

        NvBlastActorDesc actorDesc = new NvBlastActorDesc();
        actorDesc.uniformInitialBondHealth = 1.0f;
        actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
        var actor = new NvBlastActor(blastFamily, actorDesc);


        OnActorCreated(actor, Vector3.zero, Quaternion.identity);

        // Reserved buffers
        fractureBuffers = new NvBlastFractureBuffers();
        fractureBuffers.chunkFractures = Marshal.AllocHGlobal((int)chunkCount * Marshal.SizeOf(typeof(NvBlastChunkFractureData)));
        fractureBuffers.bondFractures = Marshal.AllocHGlobal((int)bondCount * Marshal.SizeOf(typeof(NvBlastBondFractureData)));
        leafChunkCount = (uint)blastAsset.leafChunkCount;
        newActorsBuffer = Marshal.AllocHGlobal((int)leafChunkCount * Marshal.SizeOf(typeof(IntPtr)));
    }

    public void Initialize(NvBlastAuthoringResult authoringResult, BreakableChunk[] chunks)
    {
        this.chunks = chunks;

        //List<BreakableChunk> allChunks = new List<BreakableChunk>();
        //List<BreakableChunk> topLevelChunks = new List<BreakableChunk>();
        //for (int c = 0; c < rootChunk.transform.childCount; c++)
        //{
        //    Transform child = rootChunk.transform.GetChild(c);
        //    BreakableChunk chunk = child.gameObject.GetComponent<BreakableChunk>();
        //    if (chunk != null)
        //    {
        //        topLevelChunks.Add(chunk);
        //    }
        //}

        //// create the blast asset description
        //List<NvBlastChunkDesc> solverChunks = new List<NvBlastChunkDesc>();
        //List<NvBlastBondDesc> solverBonds = new List<NvBlastBondDesc>();

        //// TODO: fill out the chunks and bonds

        //// Prepare solver asset desc
        //blastAssetDesc.chunkCount = (uint)solverChunks.Count;
        //blastAssetDesc.chunkDescs = solverChunks.ToArray();
        //blastAssetDesc.bondCount = (uint)solverBonds.Count;
        //blastAssetDesc.bondDescs = solverBonds.ToArray();

        //// Reorder chunks
        //uint[] chunkReorderMap = new uint[blastAssetDesc.chunkCount];
        //NvBlastExtUtilsWrapper.ReorderAssetDescChunks(blastAssetDesc, chunkReorderMap);
        //BreakableChunk[] chunksTemp = new BreakableChunk[SubChunks.Length];
        //for (uint i = 0; i < chunkReorderMap.Length; ++i)
        //{
        //    SubChunks[(int)chunkReorderMap[i]] = chunksTemp[i];
        //}


        ///////////////////////////////////////////////
        //// TRY TO FRACTURE AND LOAD THE RESULT
        ///////////////////////////////////////////////
        /*
        blastAssetDesc = new NvBlastAssetDesc();
        blastAssetDesc.bondCount = (uint)authoringResult.GetBondCount();
        blastAssetDesc.chunkCount = (uint)authoringResult.GetChunkCount();
        NvBlastBondDesc[] bondDescs = new NvBlastBondDesc[blastAssetDesc.bondCount];
        for (int b = 0; b < bondDescs.Length; b++) {
            bondDescs[b] = authoringResult.GetBondDesc(b);
        }
        blastAssetDesc.bondDescs = bondDescs;
        NvBlastChunkDesc[] chunkDescs = new NvBlastChunkDesc[blastAssetDesc.chunkCount];
        for (int c = 0; c < chunkDescs.Length; c++) {
            chunkDescs[c] = authoringResult.GetChunkDesc(c);
            chunkDescs[c].userData = (uint)chunks[c].GetInstanceID();

            chunks[c].Initialize(chunkDescs[c]);
        }
        blastAssetDesc.chunkDescs = chunkDescs;

        blastAsset = new NvBlastAsset(blastAssetDesc);
        

        // First actor
        blastFamily = new NvBlastFamily(blastAsset);

        NvBlastActorDesc actorDesc = new NvBlastActorDesc();
        actorDesc.uniformInitialBondHealth = 1.0f;
        actorDesc.uniformInitialLowerSupportChunkHealth = 1.0f;
        var actor = new NvBlastActor(blastFamily, actorDesc);
        

        OnActorCreated(actor, Vector3.zero, Quaternion.identity);

        // Reserved buffers
        fractureBuffers = new NvBlastFractureBuffers();
        fractureBuffers.chunkFractures = Marshal.AllocHGlobal((int)blastAssetDesc.chunkCount * Marshal.SizeOf(typeof(NvBlastChunkFractureData)));
        fractureBuffers.bondFractures = Marshal.AllocHGlobal((int)blastAssetDesc.bondCount * Marshal.SizeOf(typeof(NvBlastBondFractureData)));
        leafChunkCount = (uint)blastAsset.leafChunkCount;
        newActorsBuffer = Marshal.AllocHGlobal((int)leafChunkCount * Marshal.SizeOf(typeof(IntPtr)));
        */

    }

    private void OnActorCreated(NvBlastActor actor, Vector3 localPosition, Quaternion localRotation)
    {
        var rigidBodyGO = new GameObject("RigidActor ");
        rigidBodyGO.transform.SetParent(this.transform, false);
        var rigidbody = rigidBodyGO.AddComponent<Rigidbody>();
        rigidbody.transform.localPosition = localPosition;
        rigidbody.transform.localRotation = localRotation;

        // chunks
        var chunkIndices = actor.visibleChunkIndices;
        foreach (var chunkIndex in chunkIndices)
        {
            var chunkCube = chunks[chunkIndex];
            chunkCube.transform.SetParent(rigidbody.transform, false);
            chunkCube.gameObject.SetActive(true);
            rigidBodyGO.name += "," + chunkIndex;
        }

        // search for static chunks
        var graphNodeIndices = actor.graphNodeIndices;
        var chunkGraph = blastAsset.chunkGraph;
        foreach (var node in graphNodeIndices)
        {
            var chunkIndex = Marshal.ReadInt32(chunkGraph.chunkIndices, Marshal.SizeOf(typeof(UInt32)) * (int)node);
            var chunkCube = chunks[chunkIndex];
            if (chunkCube.IsStatic)
            {
                rigidbody.isKinematic = true;
                break;
            }
        }

        actor.userData = rigidbody;
        actors.Add(rigidbody, actor);
    }

    private void OnActorDestroyed(NvBlastActor actor)
    {
        var chunkIndices = actor.visibleChunkIndices;
        foreach (var chunkIndex in chunkIndices)
        {
            var chunkCube = chunks[chunkIndex];
            chunkCube.transform.SetParent(transform, false);
            chunkCube.gameObject.SetActive(false);
        }

        var rigidbody = (actor.userData as Rigidbody);
        actors.Remove(rigidbody);
        Destroy(rigidbody.gameObject);
        actor.userData = null;
    }

    public void ApplyRadialDamage(Vector3 position, float minRadius, float maxRadius, float compressive)
    {
        var hits = Physics.OverlapSphere(position, maxRadius);
        foreach (var hit in hits)
        {
            var rb = hit.GetComponentInParent<Rigidbody>();
            if (rb != null)
            {
                ApplyRadialDamage(rb, position, minRadius, maxRadius, compressive);
            }
        }
    }

    public bool ApplyRadialDamage(Rigidbody rb, Vector3 position, float minRadius, float maxRadius, float compressive)
    {
        if (actors.ContainsKey(rb))
        {
            Vector3 localPosition = rb.transform.InverseTransformPoint(position);
            Debug.Log("apply radial damage to " + rb.name + " at " + localPosition);
            ApplyRadialDamage(actors[rb], localPosition, minRadius, maxRadius, compressive);
            return true;
        }
        return false;
    }

    private void ApplyRadialDamage(NvBlastActor actor, Vector3 localPosition, float minRadius, float maxRadius, float compressive)
    {
        fractureBuffers.chunkFractureCount = (uint)chunkCount;
        fractureBuffers.bondFractureCount = (uint)bondCount;

        //NvBlastExtRadialDamageDesc desc = new NvBlastExtRadialDamageDesc();
        NvBlastExtImpactSpreadDamageDesc desc = new NvBlastExtImpactSpreadDamageDesc();

        desc.minRadius = minRadius;
        desc.maxRadius = maxRadius;
        desc.damage = compressive;
        desc.p0 = localPosition.x;
        desc.p1 = localPosition.y;
        desc.p2 = localPosition.z;

        IntPtr dam = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(NvBlastExtRadialDamageDesc)));
        Marshal.StructureToPtr(desc, dam, false);

        var damP = new NvBlastDamageProgram()
        {
            //graphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtFalloffGraphShader,
            //subgraphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtFalloffSubgraphShader
            graphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtImpactSpreadGraphShader,
            subgraphShaderFunction = NvBlastExtShadersWrapper.NvBlastExtImpactSpreadSubgraphShader
        };
        var programParams = new NvBlastExtProgramParams()
        {
            damageDescBuffer = dam,
            material = IntPtr.Zero,
            accelerator = IntPtr.Zero
        };

        actor.GenerateFracture(fractureBuffers, damP, programParams);
        actor.ApplyFracture(fractureBuffers);
        if (fractureBuffers.bondFractureCount + fractureBuffers.chunkFractureCount > 0)
        {
            Split(actor);
        }

        Marshal.FreeHGlobal(dam);
    }

    private void Split(NvBlastActor actor)
    {
        NvBlastActorSplitEvent split = new NvBlastActorSplitEvent();
        split.newActors = newActorsBuffer;
        var count = actor.Split(split, leafChunkCount);

        Vector3 localPosition = Vector3.zero;
        Quaternion localRotation = Quaternion.identity;

        if (split.deletedActor != IntPtr.Zero)
        {
            if (actor.userData != null)
            {
                var parentRigidbody = (actor.userData as Rigidbody);
                localPosition = parentRigidbody.transform.localPosition;
                localRotation = parentRigidbody.transform.localRotation;
            }
            OnActorDestroyed(actor);
        }
        for (int i = 0; i < count; i++)
        {
            int elementSize = Marshal.SizeOf(typeof(IntPtr));
            var ptr = Marshal.ReadIntPtr(split.newActors, elementSize * i);
            OnActorCreated(new NvBlastActor(blastFamily, ptr), localPosition, localRotation);
        }
    }

    private void OnDestroy()
    {
        if (fractureBuffers != null)
        {
            Marshal.FreeHGlobal(fractureBuffers.chunkFractures);
            Marshal.FreeHGlobal(fractureBuffers.bondFractures);
            fractureBuffers = null;
        }

        if (newActorsBuffer != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(newActorsBuffer);
        }

        actors.Clear();
        blastFamily = null;
        blastAsset = null;
    }
}
