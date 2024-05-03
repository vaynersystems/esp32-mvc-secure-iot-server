// define vars                                                         
var tokenUrl = '/login';

var token_ // variable will store the token

var request = new XMLHttpRequest();

function getToken(url, user, pass) {
    var key;
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.setRequestHeader("Authorization", "Basic " + btoa(user + ":" + pass));
    request.setRequestHeader("X_RETURN_URL", "");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            if (request.status == 401) {
                document.getElementById('error').appendChild('<p class="error">' + request.responseText + '</p>');
                return;
            }
            var response = request.responseText;
            var obj = JSON.parse(response);
            key = obj.access_token; //store the value of the accesstoken
            token_ = key; // store token in your global variable "token_" or you could simply return the value of the access token from the function

            request.token = token_;
            //window.sessionStorage.token = token_;
            clearCookies();
            setCookie('auth',token_.substring(7));
            //document.cookie = token_.substring(7);
            const urlParams = new URLSearchParams(window.location.search);
            const redirectTo = urlParams.get('to');
            if(redirectTo !== undefined && redirectTo !== null){
                window.document.location = redirectTo;
            } else
                window.document.location = "/";
        }
    }

    request.send("{\"user_id\":\"" + user + "\",\"password\":\"" + pass + "\"}"); // specify the credentials to receive the token on request


}

function readToken() {
    let storageToken = window.sessionStorage.token;
    if(storageToken === undefined) return;
    let authParts = storageToken.split(' ');
    if(authParts.length < 2) return;
    let token = {};
    token.raw = authParts[1];
    token.header = JSON.parse(window.atob(token.raw.split('.')[0]));
    token.payload = JSON.parse(window.atob(token.raw.split('.')[1]));
    return (token)
}

function clearCookies(){
    var cookies = document.cookie.split(';');
    for(i in cookies){
        var vals = cookies[i].split('=');
        var name = vals.shift(0, 1).trim();
        document.cookie = name+'=';
    }
}

function setCookie(path, value){
    var cookies = document.cookie.split(';');
    for(i in cookies){
        var vals = cookies[i].split('=');
        var name = vals.shift(0, 1).trim();
        if(name == path){
            document.cookie = name + '=' + value;
            return;
        }
    }
    //didn't find it
    document.cookie = path + "=" + value;
}

function getUsername(){
    var userToken = readToken();
    if(userToken !== undefined){
        return userToken.payload.user;
    }
    return '';
}

function getRole(){
    var userToken = readToken();
    if(userToken !== undefined){
       return userToken.payload.role;
    }
    return '';
}

function loadToken() {
    console.log('Token: ' + window.sessionStorage.token);
    return window.sessionStorage.token;
}