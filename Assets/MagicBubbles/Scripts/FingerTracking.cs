using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class FingerTracking : MonoBehaviour {

    public Transform LeftIndexTip;
    public Transform LeftThumbTip;
    public Transform LeftPinkyTip;
    public Transform LeftCenter;
    public Transform LeftWrist;
    public Transform RightIndexTip;
    public Transform RightThumbTip;
    public Transform RightPinkyTip;
    public Transform RightCenter;
    public Transform RightWrist;

    private MLHandKeyPose[] _gestures;
    private List<MeshRenderer> _renderes;
    private SphereCollider _leftFingerCollider;
    private MeshCollider _leftCenterCollider;
    private SphereCollider _rightFingerCollider;
    private MeshCollider _rightCenterCollider;

    private const float CenterOffset = -0.05f;
    private const float ConfidenceThreshold = 0.9f;

    public void ToogleTrackerVisibility(bool show)
    {
        foreach (var meshRenderer in _renderes) {
            meshRenderer.enabled = show;
        }
    }

    private void Awake()
    {
        MLHands.Start();

        // seems you need at least one pose to track keypoints
        _gestures = new MLHandKeyPose[1];
        _gestures[0] = MLHandKeyPose.Finger;
        MLHands.KeyPoseManager.EnableKeyPoses(_gestures, true);

        _renderes = new List<MeshRenderer>
        {
            LeftIndexTip.gameObject.GetComponent<MeshRenderer>(),
            LeftThumbTip.gameObject.GetComponent<MeshRenderer>(),
            LeftPinkyTip.gameObject.GetComponent<MeshRenderer>(),
            LeftCenter.gameObject.GetComponent<MeshRenderer>(),
            LeftWrist.gameObject.GetComponent<MeshRenderer>(),
            RightIndexTip.gameObject.GetComponent<MeshRenderer>(),
            RightThumbTip.gameObject.GetComponent<MeshRenderer>(),
            RightPinkyTip.gameObject.GetComponent<MeshRenderer>(),
            RightCenter.gameObject.GetComponent<MeshRenderer>(),
            RightWrist.gameObject.GetComponent<MeshRenderer>()
        };

        _leftFingerCollider = LeftIndexTip.gameObject.GetComponent<SphereCollider>();
        _leftCenterCollider = LeftCenter.gameObject.GetComponent<MeshCollider>();
        _rightFingerCollider = RightIndexTip.gameObject.GetComponent<SphereCollider>();
        _rightCenterCollider = RightCenter.gameObject.GetComponent<MeshCollider>();
    }

    private void OnDestroy() {
        MLHands.Stop();
    }

    private void Update()
    {
        if (MLHands.Left.HandConfidence > ConfidenceThreshold) {
            LeftIndexTip.position = MLHands.Left.Index.Tip.Position;
            LeftThumbTip.position = MLHands.Left.Thumb.Tip.Position;
            LeftPinkyTip.position = MLHands.Left.Pinky.Tip.Position;
            LeftCenter.position = MLHands.Left.Center;
            LeftWrist.position = MLHands.Left.Wrist.Center.Position;
            LeftCenter.LookAt(LeftWrist);
            LeftCenter.position = Vector3.MoveTowards(LeftCenter.position, LeftWrist.position, CenterOffset);
        }

        if (MLHands.Right.HandConfidence > ConfidenceThreshold) {
            RightIndexTip.position = MLHands.Right.Index.Tip.Position;
            RightThumbTip.position = MLHands.Right.Thumb.Tip.Position;
            RightPinkyTip.position = MLHands.Right.Pinky.Tip.Position;
            RightCenter.position = MLHands.Right.Center;
            RightWrist.position = MLHands.Right.Wrist.Center.Position;
            RightCenter.LookAt(RightWrist);
            RightCenter.position = Vector3.MoveTowards(RightCenter.position, RightWrist.position, CenterOffset);
        }

        var leftPoseConfident = MLHands.Left.KeyPoseConfidence > ConfidenceThreshold;
        var leftPointing = MLHands.Left.KeyPose.Equals(MLHandKeyPose.Finger) && leftPoseConfident;
        _leftFingerCollider.enabled = leftPointing;
        _leftCenterCollider.enabled = !leftPointing;
        _renderes[0].material.color = leftPointing ? Color.red : Color.green;

        var rightPoseConfident = MLHands.Right.KeyPoseConfidence > ConfidenceThreshold;
        var rightPointing = MLHands.Right.KeyPose.Equals(MLHandKeyPose.Finger) && rightPoseConfident;
        _rightFingerCollider.enabled = rightPointing;
        _rightCenterCollider.enabled = !rightPointing;
        _renderes[5].material.color = rightPointing ? Color.red : Color.green;
    }

}
