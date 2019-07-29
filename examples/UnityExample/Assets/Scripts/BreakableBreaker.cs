using System.Collections;
using UnityEngine;

public class BreakableBreaker : MonoBehaviour
{
    private Breakable breakable;

    public float MinRadius = 0f;
    public float MaxRadius = 25f;
    [Range(0,1)]
    public float Compressive = 1f;
    public float Explosive = 300f;

    public enum ExplosionTypes { Explode, Forward, Backward };
    public ExplosionTypes ExplosionType = ExplosionTypes.Explode;

    private Vector3 hitPoint;
    private float hitAt;

    void Awake()
    {
        breakable = GetComponent<Breakable>();
    }

    void Update()
    {
        if (Input.GetMouseButtonDown(0))
        {
            StartCoroutine(DoDamage());
        }
    }

    IEnumerator DoDamage()
    {
        Ray ray = Camera.main.ScreenPointToRay(Input.mousePosition);
        RaycastHit hitInfo;
        if (Physics.Raycast(ray, out hitInfo))
        {
            hitPoint = hitInfo.point;
            hitAt = Time.time;
            breakable.ApplyRadialDamage(hitInfo.point, MinRadius, MaxRadius, Compressive);//, Explosive);


            yield return null;

            Collider[] hits = Physics.OverlapSphere(hitInfo.point, MinRadius);
            foreach (var hit in hits)
            {
                var rb = hit.GetComponentInParent<Rigidbody>();
                if (rb != null)
                {
                    if (ExplosionType == ExplosionTypes.Explode)
                    {
                        rb.AddExplosionForce(Explosive, hitInfo.point, MaxRadius, 3.0f);
                    }
                    else if (ExplosionType == ExplosionTypes.Forward)
                    {
                        rb.AddForce(ray.direction * -Explosive, ForceMode.Impulse);
                        Debug.DrawRay(rb.position + rb.centerOfMass, ray.direction, Color.red, 1f);
                    }
                    else if (ExplosionType == ExplosionTypes.Backward)
                    {
                        rb.AddForce(ray.direction * Explosive, ForceMode.Impulse);
                        Debug.DrawRay(rb.position + rb.centerOfMass, ray.direction, Color.red, 1f);
                    }
                }
            }
        }
    }

    void OnDrawGizmos()
    {
        if (Time.time - hitAt < 1f)
        {
            Gizmos.color = Color.red;
            Gizmos.DrawSphere(hitPoint, MaxRadius);
        }
    }
}
