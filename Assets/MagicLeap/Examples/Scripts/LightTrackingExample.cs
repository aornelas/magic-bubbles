// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

using UnityEngine;
using UnityEngine.UI;

using UnityEngine.XR.MagicLeap;

namespace MagicLeap
{
    /// <summary>
    /// This class handles the functionality of updating the scene based on the lighting conditions.
    /// This includes the intensity, ambient intensity & color, and also updates the animation
    /// of a plant model which react to the light intensity changes.
    /// The UI also displays the detected light intensity level indicated in the top right.
    /// </summary>
    public class LightTrackingExample : MonoBehaviour
    {
        #region Private Variables
        [SerializeField, Tooltip("The primary light that is used in the scene.")]
        private Light _light;

        [SerializeField, Tooltip("The image (filled) that is used to display the light intensity.")]
        private Image _lightIntensity;

        [SerializeField, Tooltip("The model animator that will update based on the light intensity.")]
        private Animator _animator;

        [SerializeField, Tooltip("The cursor used to visualize the location of the plant.")]
        private Transform _raycastCursor;

        [SerializeField, Tooltip("The transform of the plant model used in the scene.")]
        private Transform _plantModel;

        private Color _color;
        private float _normalizedLuminance;
        private float _maxLuminance = 0;
        #endregion

        #region Unity Methods
        private void Start()
        {
            if (_light == null)
            {
                Debug.LogError("Error: LightTrackingExample._light is not set, disabling script.");
                enabled = false;
                return;
            }

            if (_lightIntensity == null)
            {
                Debug.LogError("Error: LightTrackingExample._lightIntensity is not set, disabling script.");
                enabled = false;
                return;
            }

            if (_animator == null)
            {
                Debug.LogError("Error: LightTrackingExample._animator is not set, disabling script.");
                enabled = false;
                return;
            }

            if (_plantModel == null)
            {
                Debug.LogError("Error: LightTrackingExample._plantModel is not set, disabling script.");
                enabled = false;
                return;
            }

            if (_raycastCursor == null)
            {
                Debug.LogError("Error: LightTrackingExample._raycastCursor is not set, disabling script.");
                enabled = false;
                return;
            }

            MLResult result = MLLightingTracker.Start();
            if (result.IsOk)
            {
                enabled = true;
                RenderSettings.ambientLight = Color.black;
            }

            MLInput.OnControllerButtonDown += HandleOnButtonDown;
        }

        void OnDestroy()
        {
            MLInput.OnControllerButtonDown -= HandleOnButtonDown;

            if (MLLightingTracker.IsStarted)
            {
                MLLightingTracker.Stop();
            }
        }

        void Update()
        {
            if (MLLightingTracker.IsStarted)
            {
                // Set the maximum observed luminance, so we can normalize it.
                if (_maxLuminance < (MLLightingTracker.AverageLuminance / 3.0f))
                {
                    _maxLuminance = (MLLightingTracker.AverageLuminance / 3.0f);
                }

                _color = MLLightingTracker.GlobalTemperatureColor;
                _normalizedLuminance = (float)(System.Math.Min(System.Math.Max((double)MLLightingTracker.AverageLuminance, 0.0), _maxLuminance) / _maxLuminance);

                RenderSettings.ambientLight = _color;
                RenderSettings.ambientIntensity = _normalizedLuminance;

                // Adjust the light for the plant animation.
                _animator.SetFloat("Sunlight", _normalizedLuminance);

                // Set the light intensity of the scene light.
                if (_light != null)
                {
                    _light.intensity = _normalizedLuminance;
                }

                // Set the light intensity meter.
                if (_lightIntensity != null)
                {
                    _lightIntensity.fillAmount = _normalizedLuminance;
                }
            }
        }
        #endregion

        #region Event Handlers
        /// <summary>
        /// Toggles the placement of the plant model.
        /// </summary>
        /// <param name="controllerId"></param>
        /// <param name="button"></param>
        private void HandleOnButtonDown(byte controllerId, MLInputControllerButton button)
        {
            if (button == MLInputControllerButton.Bumper)
            {
                if(_plantModel.transform.parent == null)
                {
                    _raycastCursor.gameObject.SetActive(true);

                    _plantModel.transform.SetParent(_raycastCursor);
                    _plantModel.transform.localPosition = Vector3.zero;
                    _plantModel.transform.localRotation = Quaternion.Euler(90, 0, 0);
                }
                else if (_plantModel.transform.parent == _raycastCursor)
                {
                    _raycastCursor.gameObject.SetActive(false);

                    _plantModel.transform.SetParent(null);
                }
            }
        }
        #endregion
    }
}
