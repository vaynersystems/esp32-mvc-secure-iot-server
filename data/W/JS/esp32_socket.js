class esp32_socket {

    socket = undefined;  
    service = '';  

    constructor(name){
        this.service = name;
    }

    connect(onServerMessage){
        this.onServerMessage = onServerMessage;
        this.closing = false;
        this.retryDelay = this.retryDelay || 2000;
        // Leaving or reloading the page closes the socket cleanly, so the device frees the
        // connection right away instead of keeping a slot for a page that no longer exists.
        if (!this.unloadHooked) {
            window.addEventListener('pagehide', () => this.disconnect());
            this.unloadHooked = true;
        }
        const baseUri = (location.protocol == 'https:' ? 'wss' : 'ws') + '://' + location.host ;
        this.socket = new WebSocket( baseUri + '/socket');  
        this.socket.addEventListener("open", (event) => {
            this.retryDelay = 2000;
            this.send("ping");
        });  
        this.socket.addEventListener("message", (event) => {
            onServerMessage(event);            
        });    
        // The device may close the socket (restart, or to make room for another client):
        // reconnect with a growing delay while the page is still open.
        this.socket.addEventListener("close", () => {
            if (this.closing) return;
            clearTimeout(this.retryTimer);
            this.retryTimer = setTimeout(() => {
                if (!this.closing && document.visibilityState !== 'hidden') this.connect(this.onServerMessage);
                else if (!this.closing) document.addEventListener('visibilitychange', () => {
                    if (document.visibilityState === 'visible' && !this.closing) this.connect(this.onServerMessage);
                }, { once: true });
            }, this.retryDelay);
            this.retryDelay = Math.min(this.retryDelay * 2, 60000);
        });
    }
    
    disconnect(){
        this.closing = true;
        clearTimeout(this.retryTimer);
        if (this.socket) this.socket.close();
    }
    
    send(message){
        if(this.socket.readyState != 1) return false;
        var request = {};
        request.service = this.service;
        request.message = message;
        this.socket.send(JSON.stringify( request));
        this.count++;
        return true;
    }
    
}