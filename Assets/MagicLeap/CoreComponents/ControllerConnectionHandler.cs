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

using System;
using System.Collections.Generic;
using UnityEngine;

namespace UnityEngine.XR.MagicLeap
{
    /// <summary>
    /// Class to automatically handle connection/disconnection events of an input device. By default,
    /// all device types are allowed but it could be modified through the inspector to limit which types to
    /// allow. This class automatically handles the disconnection/reconnection of Control and MCA devices.
    /// This class keeps track of all connected devices matching allowed types. If more than one allowed
    /// device is connected, the first one connected is returned.
    /// </summary>
    public sealed class ControllerConnectionHandler : MonoBehaviour
    {
        #region Public Enum
        /// <summary>
        /// Flags to determine which input devices to allow
        /// </summary>
        [Flags]
        public enum DeviceTypesAllowed : int
        {
            MobileApp = 1 << 0,
            ControllerLeft = 1 << 1,
            ControllerRight = 1 << 2,
        }
        #endregion

        #region Private Variables
        [SerializeField, BitMask(typeof(DeviceTypesAllowed)), Tooltip("Bitmask on which devices to allow.")]
        private DeviceTypesAllowed _devicesAllowed = (DeviceTypesAllowed)~0;

        private List<MLInputController> _allowedConnectedDevices = new List<MLInputController>();
        #endregion

        #region Public Variables
        /// <summary>
        /// Getter for the first allowed connected device, could return null.
        /// </summary>
        public MLInputController ConnectedController
        {
            get
            {
                return (_allowedConnectedDevices.Count == 0) ? null : _allowedConnectedDevices[0];
            }
        }
        #endregion

        #region Public Events
        /// <summary>
        /// Invoked only when the current controller was invalid and a controller attempted to connect.
        /// First parameter is the newly connected controller if allowed, otherwise null.
        /// </summary>
        public System.Action<MLInputController> OnControllerConnected;

        /// <summary>
        /// Invoked only when the current controller disconnects.
        /// </summary>
        public System.Action<MLInputController> OnControllerDisconnected;
        #endregion

        #region Unity Methods
        /// <summary>
        /// Starts the MLInput, initializes the first controller, and registers the connection handlers
        /// </summary>
        private void Awake()
        {
            MLInputConfiguration config = new MLInputConfiguration(MLInputConfiguration.DEFAULT_TRIGGER_DOWN_THRESHOLD,
                                                        MLInputConfiguration.DEFAULT_TRIGGER_UP_THRESHOLD,
                                                        true);
            MLResult result = MLInput.Start(config);
            if (!result.IsOk)
            {
                Debug.LogErrorFormat("Error: ControllerConnectionHandler failed starting MLInput, disabling script. Reason: {0}", result);
                enabled = false;
                return;
            }

            RegisterConnectionHandlers();
            GetAllowedInput();
        }

        /// <summary>
        /// Unregisters the connection handlers and stops the MLInput
        /// </summary>
        private void OnDestroy()
        {
            if (MLInput.IsStarted)
            {
                UnregisterConnectionHandlers();
                MLInput.Stop();
            }
        }
        #endregion

        #region Private Methods
        /// <summary>
        /// Registers the on connection/disconnection handlers.
        /// </summary>
        private void RegisterConnectionHandlers()
        {
            MLInput.OnControllerConnected += HandleOnControllerConnected;
            MLInput.OnControllerDisconnected += HandleOnControllerDisconnected;
        }

        /// <summary>
        /// Unregisters the on connection/disconnection handlers.
        /// </summary>
        private void UnregisterConnectionHandlers()
        {
            MLInput.OnControllerDisconnected -= HandleOnControllerDisconnected;
            MLInput.OnControllerConnected -= HandleOnControllerConnected;
        }

        /// <summary>
        /// Fills allowed connected devices list with all the connected controllers matching
        /// types set in _devicesAllowed.
        /// </summary>
        private void GetAllowedInput()
        {
            for (int i = 0; i < 2; ++i)
            {
                MLInputController controller = MLInput.GetController(i);
                if (IsDeviceAllowed(controller) && !_allowedConnectedDevices.Exists((device) => device.Id == controller.Id))
                {
                    _allowedConnectedDevices.Add(controller);
                }
            }
        }

        /// <summary>
        /// Checks if a controller exists, is connected, and is allowed.
        /// </summary>
        /// <param name="controller">The controller to be checked for</param>
        /// <returns>True if the controller exists, is connected, and is allowed</returns>
        private bool IsDeviceAllowed(MLInputController controller)
        {
            if (controller == null || !controller.Connected)
            {
                return false;
            }

            return (((_devicesAllowed & DeviceTypesAllowed.MobileApp) != 0 && controller.Type == MLInputControllerType.MobileApp) ||
                ((_devicesAllowed & DeviceTypesAllowed.ControllerLeft) != 0 && controller.Type == MLInputControllerType.Control && controller.Hand == MLInput.Hand.Left) ||
                ((_devicesAllowed & DeviceTypesAllowed.ControllerRight) != 0 && controller.Type == MLInputControllerType.Control && controller.Hand == MLInput.Hand.Right));
        }
        #endregion

        #region Public Methods
        /// <summary>
        /// Checks if there is a controller in the list. This method
        /// does not check if the controller is of the allowed device type
        /// since that's handled by the connection/disconnection handlers.
        /// Should not be called from Awake() or OnEnable().
        /// </summary>
        /// <returns>True if the controller is ready for use, false otherwise</returns>
        public bool IsControllerValid()
        {
            return (ConnectedController != null);
        }

        /// <summary>
        /// Checks if controller list contains controller with input id.
        /// This method does not check if the controller is of the allowed device
        /// type since that's handled by the connection/disconnection handlers.
        /// Should not be called from Awake() or OnEnable().
        /// </summary>
        /// <param name="controllerId"> Controller id to check against </param>
        /// <returns>True if a controller is found, false otherwise</returns>
        public bool IsControllerValid(byte controllerId)
        {
            return _allowedConnectedDevices.Exists((device) => device.Id == controllerId);
        }
        #endregion

        #region Event Handlers
        /// <summary>
        /// Handles the event when a controller connects. If the connected controller
        /// is valid, we add it to the _allowedConnectedDevices list.
        /// </summary>
        /// <param name="controllerId">The id of the controller.</param>
        private void HandleOnControllerConnected(byte controllerId)
        {
            MLInputController newController = MLInput.GetController(controllerId);
            if (IsDeviceAllowed(newController))
            {
                if(_allowedConnectedDevices.Exists((device) => device.Id == controllerId))
                {
                    Debug.LogWarning(string.Format("Connected controller with id {0} already connected.", controllerId));
                    return;
                }

                _allowedConnectedDevices.Add(newController);
            }
        }

        /// <summary>
        /// Handles the event when a controller disconnects. If the disconnected
        /// controller happens to be in the _allowedConnectedDevices list, we
        /// remove it from the list.
        /// </summary>
        /// <param name="controllerId">The id of the controller.</param>
        private void HandleOnControllerDisconnected(byte controllerId)
        {
            _allowedConnectedDevices.RemoveAll((device) => device.Id == controllerId);
        }
        #endregion
    }

}
