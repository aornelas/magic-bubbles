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
            _renderer = EyeGaze.GetComponent<MeshRenderer>();

            if (!MagicLeapDevice.IsReady()) {
                Debug.LogWarning("Disabling MagicBubbles.Scripts.EyeTracking because MagicLeapDevice wasn't ready.");
//                enabled = false;
                return;
            }

            MLEyes.Start();
            if (!MLEyes.IsStarted) {
                Debug.LogWarning("Disabling MagicBubbles.Scripts.EyeTracking because MLEyes didn't start.");
//                enabled = false;
            }
        }

        public void ToggleTrackerVisibility(bool show)
        {
            if (_renderer == null) {
                Debug.LogWarning("Ignoring null _renderer");
                return;
            }
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
