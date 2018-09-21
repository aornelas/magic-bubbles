using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
    public class EyeTracking : MonoBehaviour
    {
        public Transform EyeGaze;
        public TelekinesisController TelekinesisController;

        private MeshRenderer _renderer;
        private LineRenderer _gazeRay;

        private const float ConfidenceThreshold = 0.9f;
        private const float GazeRayWidth = 0.01f;

        private void Awake()
        {
            _renderer = EyeGaze.GetComponent<MeshRenderer>();
            _gazeRay = gameObject.AddComponent<LineRenderer>();
            _gazeRay.startWidth = GazeRayWidth;
            _gazeRay.endWidth = GazeRayWidth;

            if (!MagicLeapDevice.IsReady()) {
                Debug.LogWarning("Disabling MagicBubbles.Scripts.EyeTracking because MagicLeapDevice wasn't ready.");
                return;
            }

            MLEyes.Start();
            if (!MLEyes.IsStarted) {
                Debug.LogWarning("Disabling MagicBubbles.Scripts.EyeTracking because MLEyes didn't start.");
            }
        }

        public void ToggleTrackerVisibility(bool show)
        {
            if (_renderer == null) {
                Debug.LogWarning("Ignoring null _renderer");
                return;
            }
            _renderer.enabled = show;

            if (_gazeRay == null) {
                Debug.LogWarning("Ignoring null _gazeRay");
                return;
            }
            _gazeRay.enabled = show;
        }

        private void Update()
        {
            if (MLEyes.IsStarted && MLEyes.FixationConfidence > ConfidenceThreshold)
                EyeGaze.position = MLEyes.FixationPoint;

            var raySource = MLEyes.IsStarted
                ? MLEyes.LeftEye.Center - MLEyes.RightEye.Center
                : Vector3.zero + new Vector3(0, 0.1f, 0);
            var rayDir = EyeGaze.position - raySource;
            _gazeRay.SetPositions(new[] { raySource, EyeGaze.position });

            RaycastHit hit;
            if (Physics.Raycast(raySource, rayDir, out hit, 10))
            {
                if (hit.collider.CompareTag("Bubble")) {
                    TelekinesisController.GazedAtBubble(hit.collider.gameObject);
                }
            }
        }

        private void OnDestroy()
        {
            if (MLEyes.IsStarted)
                MLEyes.Stop();
        }
    }
}
