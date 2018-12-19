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
using UnityEngine.XR.MagicLeap;

namespace MagicLeap
{
    /// <summary>
    /// Manages the visual placement indicator upon attempting to place content.
    /// </summary>
    [RequireComponent(typeof(Placement))]
    public class PlacementVisualizer : MonoBehaviour
    {
        [SerializeField, Tooltip("A visual to be displayed when the volume will fit.  This transform's scale should be 1, 1, 1.")]
        private GameObject _willFitVolume;

        [SerializeField, Tooltip("A visual to be displayed when the volume will not fit.  This transform's scale should be 1, 1, 1.")]
        private GameObject _willNotFitVolume;

        private Placement _placement;

        #region Unity Methods
        void Awake()
        {
            if (_willFitVolume == null)
            {
                Debug.LogError("Error: PlacementVisualizer._willFitVolume is not set, disabling script.");
                gameObject.SetActive(false);
                return;
            }

            if (_willNotFitVolume == null)
            {
                Debug.LogError("Error: PlacementVisualizer._willNotFitVolume is not set, disabling script.");
                gameObject.SetActive(false);
                return;
            }

            _placement = GetComponent<Placement>();

            // Hide the visuals.
            _willFitVolume.SetActive(false);
            _willNotFitVolume.SetActive(false);
        }

        void Update()
        {
            // Apply the volume scale.
            _willFitVolume.transform.localScale = _placement.Scale;
            _willNotFitVolume.transform.localScale = _placement.Scale;

            if (_placement.Fit == FitType.Fits)
            {
                _willFitVolume.SetActive(true);
                _willNotFitVolume.SetActive(false);
            }
            else
            {
                _willFitVolume.SetActive(false);
                _willNotFitVolume.SetActive(true);
            }

            // Position the volume visuals.
            if (_placement.Fit == FitType.NoSurface)
            {
                _willFitVolume.transform.position = _placement.Position;
                _willNotFitVolume.transform.position = _placement.Position;
                _willFitVolume.transform.rotation = _placement.Rotation;
                _willNotFitVolume.transform.rotation = _placement.Rotation;
            }
            else
            {
                Vector3 finalLocation = Vector3.zero;
                if (_placement.MatchGravityOnVerticals && _placement.Surface == UnityEngine.XR.MagicLeap.SurfaceType.Vertical)
                {
                    finalLocation = _placement.Position;
                }
                else
                {
                    finalLocation = _placement.Position + (_placement.YAxis * _placement.Scale.y * .5f);
                }

                _willFitVolume.transform.position = finalLocation;
                _willNotFitVolume.transform.position = finalLocation;
                _willFitVolume.transform.rotation = _placement.Rotation;
                _willNotFitVolume.transform.rotation = _placement.Rotation;
            }
        }

        #endregion
    }
}
