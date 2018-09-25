using UnityEngine;

namespace MagicBubbles.Scripts
{
    public class BubbleController : MonoBehaviour
    {
        public float PopDelay = 7.0f;

        private Vector3 _originalScale;
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

        public void RecordOriginalSize()
        {
            _originalScale = transform.localScale;
        }

        public void Inflate(float power)
        {
            if (power < 0.01f) return;

            var targetScale = new Vector3(_originalScale.x + power, _originalScale.y + power, _originalScale.z + power);
            Debug.Log("Inflating bubble from " + transform.localScale + " to " + targetScale + " with original scale of " + _originalScale);
            transform.localScale = targetScale;
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
