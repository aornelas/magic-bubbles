using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
    public class BubbleGunController : MonoBehaviour
    {
        public GameObject ShootingPrefab;
        public GameObject Nozzle;
        public HandTracking HandTracking;
        public EyeTracking EyeTracking;
        public float Offset = 0.1f;
        public float ShootingForce = 300f;
        public float TriggerDownThreshold = 0.1f;
        public float TriggerUpThreshold = 1.0f;
        public float MinFireRate = 0.8f;
        public float MaxFireRate = 0.1f;

        public float MinBallSize = 0.1f;
        public float MaxBallSize = 0.25f;

        private AudioSource _audio;
        private bool _playingAudio;
        private float _nextFire;
        public bool DebugOn;

        /// <summary>
        /// Handles the event for trigger down. Throws a ball in the direction of
        /// the camera's forward vector.
        /// </summary>
        /// <param name="controllerId">The id of the controller.</param>
        /// <param name="value">The amount the trigger is pulled (0-1)</param>
        private void OnTriggerDown(byte controllerId, float value)
        {
            if (!_playingAudio) {
                _audio.pitch = 0.5f + value;
                _audio.Play();
                _playingAudio = true;
            }

            if (Time.time > _nextFire) {
                _nextFire = Time.time + FireRate(value);
                ShootBubbles();
            }
        }

        /// <summary>
        /// Normalizes (and inverts) the trigger value from _minFireRate to _maxFireRate, so the fire rate increases as
        /// the trigger value increases (it's pressed further)
        ///
        /// For chart, see: http://www.wolframalpha.com/input/?i=simplify+x+*+-1+%2B+(1+%2B+.1)+from+x+%3D+.1+to+1
        /// </summary>
        /// <param name="triggerValue"></param>
        /// <returns></returns>
        private float FireRate(float triggerValue)
        {
            return (triggerValue * -1) + (MinFireRate + MaxFireRate);
        }

        private void ShootBubbles()
        {
            // TODO: Use pool object instead of instantiating new object on each trigger down.
            // Create the ball and necessary components and shoot it along raycast.
            var ball = Instantiate(ShootingPrefab);

            ball.SetActive(true);
            var ballsize = Random.Range(MinBallSize, MaxBallSize);
            ball.transform.localScale = new Vector3(ballsize, ballsize, ballsize);
            var nozPos = Nozzle.transform.position;
            ball.transform.position = new Vector3(nozPos.x, nozPos.y, nozPos.z + Offset);

            var rigidBody = ball.GetComponent<Rigidbody>();
            if (rigidBody == null) {
                rigidBody = ball.AddComponent<Rigidbody>();
            }

            rigidBody.AddForce(Nozzle.transform.forward * ShootingForce);
        }

        private void OnTriggerUp(byte controllerId, float value)
        {
            _audio.Pause();
            _playingAudio = false;
        }


        private void OnTouchpadGestureEnd(byte controllerId, MLInputControllerTouchpadGesture gesture)
        {
            if (gesture.Type.Equals(MLInputControllerTouchpadGestureType.RadialScroll)) {
                DebugOn = !DebugOn;
                ToggleTrackerVisibility(DebugOn);
            }
        }


        private void Awake()
        {
            MLInput.OnTriggerDown += OnTriggerDown;
            MLInput.OnTriggerUp += OnTriggerUp;
            MLInput.OnControllerTouchpadGestureEnd += OnTouchpadGestureEnd;

            _audio = Nozzle.GetComponent<AudioSource>();
            ToggleTrackerVisibility(DebugOn);
        }

        private void ToggleTrackerVisibility(bool show)
        {
            HandTracking.ToogleTrackerVisibility(show);
            EyeTracking.ToggleTrackerVisibility(show);
        }

        private void Destroy()
        {
            MLInput.OnTriggerDown -= OnTriggerDown;
            MLInput.OnTriggerUp -= OnTriggerUp;
            MLInput.OnControllerTouchpadGestureEnd -= OnTouchpadGestureEnd;
        }

        private void Update()
        {
            MLInput.TriggerDownThreshold = TriggerDownThreshold;
            MLInput.TriggerUpThreshold = TriggerUpThreshold;

            if (Input.GetMouseButtonDown(0)) {
                OnTriggerDown(0,0);
            }
        }
    }
}
