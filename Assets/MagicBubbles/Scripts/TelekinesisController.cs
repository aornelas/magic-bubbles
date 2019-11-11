using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
    public class TelekinesisController : MonoBehaviour
    {

        public bool Holding;
        public bool Inflating;
        public float SimulatedPower;

        private List<Rigidbody> _frozenBubbles;
        private List<BubbleController> _bubbleControllers;
        private MLHand _inflatingHand;
        private BubbleController _inflatingBubble;
        private bool _alreadyInvoked;
        private bool _poppingBubbles;

        private const float ActionThreshold = 1.0f;

        public void PopAllHeldBubbles()
        {
            if (_bubbleControllers.Count > 0) {
                Debug.Log("Popping held bubbles");
                foreach (var bubble in _bubbleControllers.Where(bubble => bubble != null))
                    bubble.Invoke("Pop", Random.Range(0.0f, 0.2f));
            }
        }

        public void StartInflatingBubble(MLHand hand)
        {
            Debug.Log("Started inflating with hand: " + (hand.Equals(MLHands.Left) ? "left" : "right"));
            Inflating = true;
            Holding = true;
            _inflatingHand = hand;
        }

        public void PopInflatingBubble(MLHand hand)
        {
            Debug.Log("Popping inflating bubble with hand: " + (hand.Equals(MLHands.Left) ? "left" : "right"));
            Inflating = false;
            if (hand == _inflatingHand && _inflatingBubble != null) {
                _inflatingBubble.Pop();
                _inflatingBubble = null;
                _inflatingHand = null;
            }
        }

        private void ClearBubbleControllers()
        {
            if (!Holding && !Inflating) {
                _bubbleControllers.Clear();
                _alreadyInvoked = false;
            }
        }

        private void ReleaseBubbles()
        {
            if (!Holding && !Inflating) {
                Debug.Log("Stopped holding");
                foreach (var bubble in _frozenBubbles) {
                    if (bubble != null) {
                        bubble.useGravity = true;
                        bubble.GetComponent<BubbleController>().ResetTTL();
                    }
                }
                _frozenBubbles.Clear();
            }
        }

        private void Start()
        {
            _frozenBubbles = new List<Rigidbody>();
            _bubbleControllers = new List<BubbleController>();
        }

        private void Update()
        {
            if (!_poppingBubbles && Input.GetKey(KeyCode.P)) {
                PopAllHeldBubbles();
                _poppingBubbles = true;
            }

            if (_poppingBubbles && !Input.GetKey(KeyCode.P))
                _poppingBubbles = false;

            if (!Holding && !Inflating && _frozenBubbles.Count > 0 && !_alreadyInvoked) {
                // Give time to do other actions
                Invoke("ReleaseBubbles", ActionThreshold);
                Invoke("ClearBubbleControllers", ActionThreshold);
                _alreadyInvoked = true;
            }

            if (Inflating) {
                CancelInvoke();
                if (_inflatingBubble != null) {
                    var power = HandTracking.GetThumbIndexDistance(_inflatingHand);
                    if (power == HandTracking.NullHand) power = SimulatedPower;
                    if (power != HandTracking.NotConfident) {
                        Debug.Log("inflation power: " + power);
                        _inflatingBubble.Inflate(power);
                    }
                }
            }
        }

        public void GazedAtBubble(GameObject bubbleGO)
        {
            if (Holding) {
                var bubble = bubbleGO.GetComponent<Rigidbody>();
                if (!_frozenBubbles.Contains(bubble)) {
                    Debug.Log("Holding another bubble");
                    bubble.useGravity = false;
                    bubble.GetComponent<BubbleController>().CancelDeath();
                    _frozenBubbles.Add(bubble);
                    _bubbleControllers.Add(bubbleGO.GetComponent<BubbleController>());
                }
            }

            if (Inflating && _inflatingBubble == null)
                _inflatingBubble = bubbleGO.GetComponent<BubbleController>();
        }
    }
}
