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
    public enum HandPoses { Ok, Finger, Thumb, OpenHandBack, Fist, NoPose };
    public HandPoses pose = HandPoses.NoPose;

    private MLHandKeyPose[] _gestures;
    private List<MeshRenderer> _renderes;

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
        _gestures[0] = MLHandKeyPose.Ok;
        MLHands.KeyPoseManager.EnableKeyPoses(_gestures, true, false);

        _renderes = new List<MeshRenderer>();
        _renderes.Add(LeftIndexTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(LeftThumbTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(LeftPinkyTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(LeftCenter.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(LeftWrist.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(RightIndexTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(RightThumbTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(RightPinkyTip.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(RightCenter.gameObject.GetComponent<MeshRenderer>());
        _renderes.Add(RightWrist.gameObject.GetComponent<MeshRenderer>());
    }

    private void OnDestroy() {
        MLHands.Stop();
    }

    private void Update()
    {
        LeftIndexTip.position = MLHands.Left.Index.Tip.Position;
        LeftThumbTip.position = MLHands.Left.Thumb.Tip.Position;
        LeftPinkyTip.position = MLHands.Left.Pinky.Tip.Position;
        LeftCenter.position = MLHands.Left.Center;
        LeftWrist.position = MLHands.Left.Wrist.Center.Position;
        RightIndexTip.position = MLHands.Right.Index.Tip.Position;
        RightThumbTip.position = MLHands.Right.Thumb.Tip.Position;
        RightPinkyTip.position = MLHands.Right.Pinky.Tip.Position;
        RightCenter.position = MLHands.Right.Center;
        RightWrist.position = MLHands.Right.Wrist.Center.Position;
    }

}
