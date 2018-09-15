using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EyeGazeController : MonoBehaviour {

    // Use this for initialization
    void Start()
    {
        Debug.Log("EyeGazeController start");
    }

    // Update is called once per frame
    void Update()
    {
    }

    private void OnTriggerEnter(Collider other)
    {
        Debug.Log("EyeGaze collided with: " + other.name);
        other.gameObject.GetComponent<Rigidbody>().useGravity = false;
    }

    private void OnTriggerExit(Collider other)
    {
        Debug.Log("EyeGaze left: " + other.name);
        other.gameObject.GetComponent<Rigidbody>().useGravity = true;
    }
}
