using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
    public class EyeTracking : MonoBehaviour
    {
        public Transform EyeGaze;

        private TelekinesisController _telekinesis;
        private LineRenderer _gazeRay;
        private MeshRenderer _gazeBall;

        // TODO: Test with 0.0f
        private const float ConfidenceThreshold = 0.9f;

        private void Awake()
        {
            _telekinesis = EyeGaze.GetComponent<TelekinesisController>();
            _gazeRay = EyeGaze.GetComponent<LineRenderer>();
            _gazeBall = EyeGaze.GetComponentInChildren<MeshRenderer>();

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
            if (_gazeRay == null) {
                Debug.LogWarning("Ignoring null _gazeRay");
                return;
            }
            _gazeRay.enabled = show;

            if (_gazeBall == null) {
                Debug.LogWarning("Ignoring null _gazeBall");
                return;
            }
            _gazeBall.enabled = show;
        }

        private void Update()
        {
            if (MLEyes.IsStarted && MLEyes.FixationConfidence > ConfidenceThreshold)
                EyeGaze.position = MLEyes.FixationPoint;

            var raySource = MLEyes.IsStarted
                ? (MLEyes.LeftEye.Center + MLEyes.RightEye.Center) / 2
                : Vector3.zero + new Vector3(0, 0.1f, 0);
            var rayDir = EyeGaze.position - raySource;
            EyeGaze.LookAt(raySource);
            _gazeRay.SetPositions(new[] { raySource, EyeGaze.position });

            RaycastHit hit;
            if (Physics.Raycast(raySource, rayDir, out hit, 10))
            {
                if (hit.collider.CompareTag("Bubble")) {
                    _telekinesis.GazedAtBubble(hit.collider.gameObject);
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
