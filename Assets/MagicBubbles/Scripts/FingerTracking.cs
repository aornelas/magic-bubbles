using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class FingerTracking : MonoBehaviour {

    public TelekinesisController TelekinesisController;
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
        var result = MLHands.Start();
        if (!result.IsOk) {
            return;
        }

        _gestures = new MLHandKeyPose[3];
        _gestures[0] = MLHandKeyPose.Finger;
        _gestures[1] = MLHandKeyPose.OpenHandBack;
        _gestures[2] = MLHandKeyPose.Fist;

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
        if (MLHands.IsStarted)
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
        var leftOpen = MLHands.Left.KeyPose.Equals(MLHandKeyPose.OpenHandBack) && leftPoseConfident;
        var leftFist = MLHands.Left.KeyPose.Equals(MLHandKeyPose.Fist) && leftPoseConfident;
        _leftFingerCollider.enabled = leftPointing;
        _leftCenterCollider.enabled = !leftPointing;
        _renderes[0].material.color = leftPointing ? Color.red : Color.green;
        _renderes[3].material.color = leftOpen ? Color.gray : Color.black;

        var rightPoseConfident = MLHands.Right.KeyPoseConfidence > ConfidenceThreshold;
        var rightPointing = MLHands.Right.KeyPose.Equals(MLHandKeyPose.Finger) && rightPoseConfident;
        var rightOpen = MLHands.Right.KeyPose.Equals(MLHandKeyPose.OpenHandBack) && rightPoseConfident;
        var rightFist = MLHands.Right.KeyPose.Equals(MLHandKeyPose.Fist) && rightPoseConfident;
        _rightFingerCollider.enabled = rightPointing;
        _rightCenterCollider.enabled = !rightPointing;
        _renderes[5].material.color = rightPointing ? Color.red : Color.green;
        _renderes[8].material.color = rightOpen ? Color.gray : Color.black;

        TelekinesisController.Holding = leftOpen || rightOpen;
        if (leftFist || rightFist)
            TelekinesisController.PopAllHeldBubbles();
    }

}
