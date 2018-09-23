using System.Collections.Generic;
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

        private const float ActionThreshold = 1.0f;

        public void PopAllHeldBubbles()
        {
            if (_bubbleControllers.Count > 0) {
                Debug.Log("Popping held bubbles");
                foreach (var bubble in _bubbleControllers)
                    if (bubble != null)
                        bubble.Pop();
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
            if (!Holding && !Inflating)
                _bubbleControllers.Clear();
        }

        private void ReleaseBubbles()
        {
            if (!Holding && !Inflating) {
                Debug.Log("Stopped holding");
                foreach (var bubble in _frozenBubbles) {
                    if (bubble != null)
                        bubble.useGravity = true;
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
            if (!Holding && !Inflating && _frozenBubbles.Count > 0) {
                // Give time to do other actions
                Invoke("ReleaseBubbles", ActionThreshold);
                Invoke("ClearBubbleControllers", ActionThreshold);
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
                    _frozenBubbles.Add(bubble);
                    _bubbleControllers.Add(bubbleGO.GetComponent<BubbleController>());
                }
            }

            if (Inflating && _inflatingBubble == null)
                _inflatingBubble = bubbleGO.GetComponent<BubbleController>();
        }
    }
}
