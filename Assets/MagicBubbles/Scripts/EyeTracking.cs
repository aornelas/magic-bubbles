using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class EyeTracking : MonoBehaviour
{
    public Transform EyeGaze;
    private MeshRenderer _renderer;

    private const float ConfidenceThreshold = 0.9f;

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
        if (MLEyes.FixationConfidence > ConfidenceThreshold)
            EyeGaze.position = MLEyes.FixationPoint;

//        EyeGaze.LookAt(gameObject.transform);
    }

    private void OnDestroy()
    {
        MLEyes.Stop();
    }
}
