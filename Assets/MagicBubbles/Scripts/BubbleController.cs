using UnityEngine;

namespace MagicBubbles.Scripts
{
    public class BubbleController : MonoBehaviour
    {

        public float PopDelay = 7.0f;

        private AudioSource _audio;
        private MeshRenderer _mesh;

        private void Awake()
        {
            _audio = GetComponent<AudioSource>();
            _mesh = GetComponent<MeshRenderer>();
        }

        private void OnCollisionEnter(Collision other)
        {
            if (!other.gameObject.CompareTag("Bubble")) {
                Invoke("Pop", PopDelay);
            }
        }

        private void OnTriggerEnter(Collider other)
        {
            if (other.gameObject.CompareTag("Finger")) {
                Pop();
            }
        }

        public void Pop()
        {
            _audio.pitch = 1 + (transform.localScale.x * -1 + 0.2f);
            _audio.Play();
            _mesh.enabled = false;
            Invoke("BeGone", 0.25f);
        }

        private void BeGone()
        {
            Destroy(gameObject);
        }
    }
}
