// define vars                                                         
var tokenUrl = '/login';

var token_ // variable will store the token



function getToken(url, user, pass, setupCode) {
    var key;
    var request = new XMLHttpRequest();
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.setRequestHeader("Authorization", "Basic " + btoa(user + ":" + pass));
    request.setRequestHeader("X_RETURN_URL", "");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            // The spinner stays up on success: the next page can take a moment to load, and
            // hiding it first left the login form looking idle in between.
            if (request.status != 200) hideLoading();
            if (request.status == 401 || request.status == 429) {
                // A new device has no users: the first login needs the setup code from its serial console.
                if (request.statusText.indexOf('Setup code required') === 0) {
                    const setupRow = document.getElementById('setup-code-row');
                    if (setupRow) setupRow.style.display = '';
                }
                const message = '<p class="error">' + (request.statusText.length > 0 ? request.statusText  : 'Unknown error' ) + '</p>';
                showModal('Authentication Error',message);
                return;
            }
            var response = request.responseText;
            var obj;
            try { obj = JSON.parse(response); } catch (e) { obj = null; }
            if (request.status != 200 || obj === null || obj.access_token === undefined) {
                hideLoading();
                showModal('Authentication Error', '<p class="error">' + (request.statusText || 'No response from the device. Please try again.') + '</p>');
                return;
            }
            token_ = obj.access_token;  // store token in your global variable "token_" or you could simply return the value of the access token from the function

            request.token = token_;
            //window.sessionStorage.token = token_;
            var expiry = new Date();
            expiry.setDate(expiry.getDate() + 7);
            clearCookies();
            setCookie('auth',token_.substring(7));
            setCookie('expires',expiry);
            //document.cookie = token_.substring(7);
            window.document.location = safeReturnPath(new URLSearchParams(window.location.search).get('to'));
        }
    }

    // JSON.stringify, so a quote or backslash in a password can't break the request.
    const body = { user_id: user, password: pass };
    if (setupCode) body.setup_code = setupCode.trim().toUpperCase();
    request.send(JSON.stringify(body)); // specify the credentials to receive the token on request


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
    // Over HTTPS the session cookie must never be sent on a plain HTTP request.
    var attributes = location.protocol === 'https:' ? '; Secure; SameSite=Strict' : '; SameSite=Strict';
    var cookies = document.cookie.split(';');
    for(i in cookies){
        var vals = cookies[i].split('=');
        var name = vals.shift(0, 1).trim();
        if(name == path){
            document.cookie = name + '=' + value + attributes;
            return;
        }
    }
    //didn't find it
    document.cookie = path + "=" + value + attributes;
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

// Where to go after login. Only a path on this device is allowed: "?to=" used to be followed
// as-is, so a link like /login?to=javascript:... ran script on this site after the admin
// logged in (stealing the session cookie), and ?to=https://other-site redirected to it.
function safeReturnPath(to){
    if (typeof to !== 'string' || to.length === 0) return '/';
    if (to[0] !== '/' || to[1] === '/' || to[1] === '\\') return '/';
    try {
        const target = new URL(to, window.location.origin);
        if (target.origin !== window.location.origin) return '/';
        return target.pathname + target.search + target.hash;
    } catch (e) {
        return '/';
    }
}
