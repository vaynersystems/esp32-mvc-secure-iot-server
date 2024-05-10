class esp32_socket {

    socket = undefined;  
    service = '';  

    constructor(name){
        this.service = name;
    }

    connect(onServerMessage){
        const baseUri = (location.protocol == 'https:' ? 'wss' : 'ws') + '://' + location.host ;
        this.socket = new WebSocket( baseUri + '/socket');  
        this.socket.addEventListener("open", (event) => {
            this.send("ping");
        });  
        this.socket.addEventListener("message", (event) => {
            onServerMessage(event);            
        });    
    }
    
    disconnect(){
        this.socket.close();
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