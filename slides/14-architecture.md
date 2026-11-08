<h1>The whole picture</h1>

One picture, one C ABI, three places

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Now lets put it all together
- Everything from create, prepare and process in one picture, the C ABI underneath it, and the three places in a plugin where we call it

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

{{MAP:show=create,prepare,process|abi}}

Note:
- This is the whole path from our callback to the engine and back
- On the left our app: the three files and the three calls
- The dashed line is the C ABI: everything right of it is the library, and the handler is all we touch
- The session belongs to one handler: the state pairs from create, and the plans, the latency, the rings and the slots from prepare
- On the right what anira keeps once per process: the pool, the queue and the loaded models, shared by every handler
- In process a block crosses the line and comes back the same way, in place, in the same buffer
- And nothing on the audio side allocates or locks: the rules from steps 5 to 7, now kept by the library

