using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.MagicLeap;

namespace MagicBubbles.Scripts
{
    public class TelekinesisController : MonoBehaviour
    {

        public bool Holding;
        public bool Inflating;

        private List<Rigidbody> _frozenBubbles;
        private List<BubbleController> _bubbleControllers;
        private MLHand _inflatingHand;
        private BubbleController _inflatingBubble;

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
            _bubbleControllers.Clear();
        }

        private void Start()
        {
            _frozenBubbles = new List<Rigidbody>();
            _bubbleControllers = new List<BubbleController>();
        }

        private void Update()
        {
            // TODO: Give some threshold to allow time between Holding and Inflating
            if (!Holding && !Inflating && _frozenBubbles.Count > 0) {
                Debug.Log("Stopped holding");
                foreach (var bubble in _frozenBubbles) {
                    if (bubble != null)
                        bubble.useGravity = true;
                }
                _frozenBubbles.Clear();

                // Give some time to pop bubbles
                Invoke("ClearBubbleControllers", 0.5f);
            }
        }

        private void OnTriggerEnter(Collider other)
        {
            if (Holding) {
                Debug.Log("Holding another bubble");
                var bubble = other.gameObject.GetComponent<Rigidbody>();
                bubble.useGravity = false;
                _frozenBubbles.Add(bubble);
                _bubbleControllers.Add(other.gameObject.GetComponent<BubbleController>());
            }

            if (Inflating) {
                if (_inflatingBubble == null)
                    _inflatingBubble = other.gameObject.GetComponent<BubbleController>();
                var power = HandTracking.GetThumbIndexDistance(_inflatingHand);
                Debug.Log("inflation power: " + power);
                _inflatingBubble.Inflate(power);
            }
        }
    }
}
