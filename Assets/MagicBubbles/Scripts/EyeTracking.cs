using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
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
            if (MLEyes.IsStarted && MLEyes.FixationConfidence > ConfidenceThreshold)
                EyeGaze.position = MLEyes.FixationPoint;
        }

        private void OnDestroy()
        {
            if (MLEyes.IsStarted)
                MLEyes.Stop();
        }
    }
}
