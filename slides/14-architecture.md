<h1>The whole picture</h1>

One picture, one C ABI, three places

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Now let's put it all together
- Everything from create, prepare and process in one picture, the C ABI underneath it, and the three places in a plugin where we call it

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

{{MAP:show=create,prepare,process|abi}}

Note:
- You have probably noticed there are really a lot of thoughts which have gone into the design... But all of this happens under the hood.
- You can just confortably define your json files and call the three calls on our inference handler class
- The dashed line is the C ABI: everything right of it is the library internals then
- The session belongs to one handler: it has the state pairs from create, and the rings, the slots and the latency calculated from prepare
- On the right is what anira keeps once per process: the pool, the queue and the loaded models, shared by every handler
- In process a block crosses the line seamlessly and nothing on the audio side allocates or locks. All real-time safety principles are fulfilled

