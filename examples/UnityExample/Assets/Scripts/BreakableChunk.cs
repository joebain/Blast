using System.Collections.Generic;
using UnityEngine;

public class BreakableChunk : MonoBehaviour
{
    public Vector3 centroid;
    public float volume;
    public bool IsStatic = false;

    public void Initialize(NvBlastChunkDesc chunk)
    {
        centroid = new Vector3(chunk.c0, chunk.c1, chunk.c2);
        volume = chunk.volume;
        IsStatic = chunk.flags == 1;
    }

    public void Initialize(NvBlastChunk chunk)
    {
        centroid = new Vector3(chunk.c0, chunk.c1, chunk.c2);
        volume = chunk.volume;
        // TODO: do we need to find out if our chunks are static some other way???
        //IsStatic = chunk.flags == 1;

        if (GetComponent<StaticChunkMarker>())
        {
            IsStatic = true;
        }
    }
}
