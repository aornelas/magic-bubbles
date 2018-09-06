using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.MagicLeap;

public class BubbleGunController : MonoBehaviour
{
    public GameObject _shootingPrefab;
    public GameObject _nozzle;
    public FingerTracking _fingerTracking;
    public EyeTracking _eyeTracking;
    public float _offset = 0.1f;
    public float _shootingForce = 300f;
    public float _triggerDownThreshold = 0.1f;
    public float _triggerUpThreshold = 1.0f;
    public float _minFireRate = 0.8f;
    public float _maxFireRate = 0.1f;

    public float MIN_BALL_SIZE = 0.1f;
    public float MAX_BALL_SIZE = 0.25f;

    private AudioSource _audio;
    private bool _playingAudio = false;
    private float _nextFire = 0f;
    public bool _debugOn = false;

    /// <summary>
    /// Handles the event for trigger down. Throws a ball in the direction of
    /// the camera's forward vector.
    /// </summary>
    /// <param name="controller_id">The id of the controller.</param>
    /// <param name="button">The button that is being released.</param>
    private void OnTriggerDown(byte controller_id, float value)
    {
        Debug.Log("Trigger Down = " + value);

        if (!_playingAudio) {
            _audio.pitch = 0.5f + value;
            _audio.Play();
            _playingAudio = true;
        }

        Debug.Log("Time = " + Time.time);
        Debug.Log("nextFire = " + _nextFire);
        if (Time.time > _nextFire) {
            _nextFire = Time.time + FireRate(value);
            ShootBubbles();
            Debug.Log("nextFire = " + _nextFire);
        }
        else {
            Debug.Log("waiting for next fire...");
        }
    }

    /// <summary>
    /// Normalizes (and inverts) the trigger value from _minFireRate to _maxFireRate, so the fire rate increases as the
    /// trigger value increases (it's pressed further)
    ///
    /// For chart, see: http://www.wolframalpha.com/input/?i=simplify+x+*+-1+%2B+(1+%2B+.1)+from+x+%3D+.1+to+1
    /// </summary>
    /// <param name="triggerValue"></param>
    /// <returns></returns>
    private float FireRate(float triggerValue)
    {
        return (triggerValue * -1) + (_minFireRate + _maxFireRate);
    }

    private void ShootBubbles()
    {
        // TODO: Use pool object instead of instantiating new object on each trigger down.
        // Create the ball and necessary components and shoot it along raycast.
        GameObject ball = Instantiate(_shootingPrefab);

        ball.SetActive(true);
        float ballsize = Random.Range(MIN_BALL_SIZE, MAX_BALL_SIZE);
        ball.transform.localScale = new Vector3(ballsize, ballsize, ballsize);
        Vector3 nozPos = _nozzle.transform.position;
        ball.transform.position = new Vector3(nozPos.x, nozPos.y, nozPos.z + _offset);

        Rigidbody rigidBody = ball.GetComponent<Rigidbody>();
        if (rigidBody == null) {
            rigidBody = ball.AddComponent<Rigidbody>();
        }

        rigidBody.AddForce(_nozzle.transform.forward * _shootingForce);
    }

    private void OnTriggerUp(byte controller_id, float value)
    {
        _audio.Pause();
        _playingAudio = false;
    }


    private void OnTouchpadGestureEnd(byte controller_id, MLInputControllerTouchpadGesture gesture)
    {
        if (gesture.Type.Equals(MLInputControllerTouchpadGestureType.RadialScroll)) {
            _debugOn = !_debugOn;
            ToggleTrackerVisibility(_debugOn);
        }
    }


    private void Awake()
    {
        MLInput.OnTriggerDown += OnTriggerDown;
        MLInput.OnTriggerUp += OnTriggerUp;
        MLInput.OnControllerTouchpadGestureEnd += OnTouchpadGestureEnd;

        _audio = _nozzle.GetComponent<AudioSource>();
        ToggleTrackerVisibility(_debugOn);
    }

    private void ToggleTrackerVisibility(bool show)
    {
        _fingerTracking.ToogleTrackerVisibility(show);
        _eyeTracking.ToggleTrackerVisibility(show);
    }

    private void Destroy()
    {
        MLInput.OnTriggerDown -= OnTriggerDown;
        MLInput.OnTriggerUp -= OnTriggerUp;
        MLInput.OnControllerTouchpadGestureEnd -= OnTouchpadGestureEnd;
    }

    void Update()
    {
        MLInput.TriggerDownThreshold = _triggerDownThreshold;
        MLInput.TriggerUpThreshold = _triggerUpThreshold;

        if (Input.GetMouseButtonDown(0)) {
            OnTriggerDown(0,0);
        }
    }
}
