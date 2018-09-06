using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class EyeTracking : MonoBehaviour
{
    public Transform EyeGaze;
    private MeshRenderer _renderer;

    private void Awake()
    {
        MLEyes.Start();
        _renderer = EyeGaze.GetComponent<MeshRenderer>();
    }

    public void ToggleTrackerVisibility(bool show)
    {
        _renderer.enabled = show;
    }

    private void Update()
    {
        EyeGaze.position = MLEyes.FixationPoint;
        EyeGaze.LookAt(gameObject.transform);
    }

    private void OnDestroy()
    {
        MLEyes.Stop();
    }
}
