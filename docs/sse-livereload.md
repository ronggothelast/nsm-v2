# SSE Livereload Protocol

Nift v2 dev server uses Server-Sent Events (SSE) for instant livereload.

## Endpoints

### `GET /__nift/livereload`

SSE endpoint. Returns `text/event-stream` with chunked transfer encoding.

**Connection:**
```
GET /__nift/livereload HTTP/1.1
Host: 127.0.0.1:8080
```

**Response:**
```
HTTP/1.1 200 OK
Content-Type: text/event-stream
Transfer-Encoding: chunked

data: connected
```

**Events:**
- `data: connected` — sent on initial connection
- `data: reload` — sent when a file changes (triggers page reload)

### `GET /__nift/livereload/poll`

Polling fallback. Returns current generation token as plain text.

**Response:**
```
HTTP/1.1 200 OK
Content-Type: text/plain

42
```

### `GET /__nift/livereload.js`

Injected client script. Auto-detects SSE support, falls back to polling.

**Client Behavior:**
1. Try to open EventSource to `/__nift/livereload`
2. On `onmessage`: reload page
3. On error (SSE unavailable): fall back to polling `/__nift/livereload/poll`
4. Polling checks every 500ms, reloads if token changes

## Client JS

The injected script (minified):

```javascript
(function(){
  function trySSE(){
    try{
      var es=new EventSource('/__nift/livereload');
      es.onmessage=function(){window.location.reload()};
      es.onerror=function(){es.close();tryPoll()};
    }catch(e){tryPoll()}
  }
  function tryPoll(){
    var last=null;
    function poll(){
      fetch('/__nift/livereload/poll',{cache:'no-store'})
        .then(function(r){return r.text()})
        .then(function(t){
          if(last===null){last=t;return}
          if(t!==last)window.location.reload()
        })
        .catch(function(){})
    }
    setInterval(poll,500);poll()
  }
  trySSE();
})();
```

## Why SSE over WebSocket

- **Simpler** — no new library needed (cpp-httplib supports chunked streaming)
- **Works behind all proxies** — standard HTTP, no upgrade required
- **No binary protocol** — plain text events
- **Automatic reconnection** — EventSource handles reconnects natively

## Proxy Compatibility

SSE works with all common proxy configurations:
- nginx: ✓ (default settings)
- Apache: ✓ (mod_proxy)
- Cloudflare: ✓
- AWS ALB: ✓ (with keep-alive)
- Docker: ✓

If SSE is blocked (rare), client automatically falls back to polling.

## Security

- SSE endpoint is only active when `--livereload` is enabled (default)
- No authentication required (local dev server only)
- Events contain no sensitive data (just "reload" signal)
