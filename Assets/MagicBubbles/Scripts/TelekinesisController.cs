using System.Collections.Generic;
using UnityEngine;

public class TelekinesisController : MonoBehaviour
{

    public bool Holding;

    private List<Rigidbody> _frozenBubbles;
    private List<BubbleController> _bubbleControllers;

    public void PopAllHeldBubbles()
    {
        foreach (var bubble in _bubbleControllers)
            if (bubble != null)
                bubble.Pop();
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
        if (!Holding) {
            foreach (var bubble in _frozenBubbles) {
                if (bubble != null)
                    bubble.useGravity = true;
            }
            _frozenBubbles.Clear();

            // Give some time to close fist
            Invoke("ClearBubbleControllers", 0.5f);
        }
    }

    private void OnTriggerEnter(Collider other)
    {
        if (Holding) {
            var bubble = other.gameObject.GetComponent<Rigidbody>();
            bubble.useGravity = false;
            _frozenBubbles.Add(bubble);
            _bubbleControllers.Add(other.gameObject.GetComponent<BubbleController>());

        }
    }
}
