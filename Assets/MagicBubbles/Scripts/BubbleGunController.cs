using MagicLeap;
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
        public float MinFireRate = 1.0f;
        public float MaxFireRate = 0.1f;
        public Color[] BubbleColors = { Color.gray };
        public ParticleSystem[] Particles;
        public ControllerFeedbackExample Controller;

        public float MinBallSize = 0.1f;
        public float MaxBallSize = 0.25f;

        private int _bubbleColorIndex = -1;
        private int _particleColorIndex = -1;
        private bool _particleMode;
        private Material _nozzleMaterial;
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
                ShootBubbles(value);
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

        private void ShootBubbles(float forceModifier)
        {
            if (_particleMode) {
                // TODO: Emit particles
            } else {
                // TODO: Use pool object instead of instantiating new object on each trigger down.
                var bubble = Instantiate(ShootingPrefab);

                // Update bubble color to match nozzle
                var color = Nozzle.GetComponent<MeshRenderer>().material.color;
                color.a = 0.2f;
                bubble.GetComponent<MeshRenderer>().material.color = color;

                bubble.SetActive(true);
                var ballsize = Random.Range(MinBallSize, MaxBallSize);
                bubble.transform.localScale = new Vector3(ballsize, ballsize, ballsize);
                bubble.GetComponent<BubbleController>().RecordOriginalSize();
                var nozPos = Nozzle.transform.position;
                bubble.transform.position = new Vector3(nozPos.x, nozPos.y, nozPos.z + Offset);

                var rigidBody = bubble.GetComponent<Rigidbody>();
                if (rigidBody == null) {
                    rigidBody = bubble.AddComponent<Rigidbody>();
                }

                rigidBody.AddForce(Nozzle.transform.forward * ShootingForce * forceModifier);
                Controller.Buzz();
            }
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

            if (gesture.Type.Equals(MLInputControllerTouchpadGestureType.Swipe)) {
                if (gesture.Direction.Equals(MLInputControllerTouchpadGestureDirection.Left)) {
                    PreviousBubbleColor();
                }
                if (gesture.Direction.Equals(MLInputControllerTouchpadGestureDirection.Right)) {
                    NextBubbleColor();
                }
                if (gesture.Direction.Equals(MLInputControllerTouchpadGestureDirection.Up)) {
                    PreviousParticle();
                }
                if (gesture.Direction.Equals(MLInputControllerTouchpadGestureDirection.Down)) {
                    NextParticle();
                }
            }
        }

        private void NextBubbleColor()
        {
            ChangeBubbleColor(+1);
        }

        private void PreviousBubbleColor()
        {
            ChangeBubbleColor(-1);
        }

        private void ChangeBubbleColor(int indexDelta)
        {
            _particleMode = false;
            _bubbleColorIndex = (_bubbleColorIndex + indexDelta) % BubbleColors.Length;
            if (_bubbleColorIndex < 0) _bubbleColorIndex = BubbleColors.Length - 1;
            _nozzleMaterial.color = BubbleColors[_bubbleColorIndex];
            Controller.Buzz();
        }

        private void NextParticle()
        {
            ChangeParticle(+1);
        }

        private void PreviousParticle()
        {
            ChangeParticle(-1);
        }

        private void ChangeParticle(int indexDelta)
        {
            _particleMode = true;
            _particleColorIndex = (_particleColorIndex + indexDelta) % Particles.Length;
            if (_particleColorIndex < 0) _particleColorIndex = Particles.Length - 1;
//            _nozzleMaterial.color = Particles[_particleColorIndex];
            Controller.Buzz();
        }

        private void Awake()
        {
            MLInput.OnTriggerDown += OnTriggerDown;
            MLInput.OnTriggerUp += OnTriggerUp;
            MLInput.OnControllerTouchpadGestureEnd += OnTouchpadGestureEnd;

            _audio = Nozzle.GetComponent<AudioSource>();
            _nozzleMaterial = Nozzle.GetComponent<MeshRenderer>().material;
            NextBubbleColor();
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

        public float triggerValue = 1.0f;
        private void Update()
        {
            MLInput.TriggerDownThreshold = TriggerDownThreshold;
            MLInput.TriggerUpThreshold = TriggerUpThreshold;

            if (Input.GetMouseButtonDown(0)) {
                OnTriggerDown(0,triggerValue);
            }
        }
    }
}
