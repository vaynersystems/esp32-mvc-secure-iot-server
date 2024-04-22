// define vars                                                         
var tokenUrl = '/login';

var token_ // variable will store the token

var request = new XMLHttpRequest();

function getToken(url, user, pass) {
    var key;
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.setRequestHeader("Authorization", "Basic " + btoa(user + ":" + pass));
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            if (request.status == 401) {
                document.getElementById('login').appendChild('<p class="error">Error occured: ' + request.responseText + '</p>');
            }
            var response = request.responseText;
            var obj = JSON.parse(response);
            key = obj.access_token; //store the value of the accesstoken
            token_ = key; // store token in your global variable "token_" or you could simply return the value of the access token from the function

            request.token = token_;
            window.sessionStorage.token = token_;
            document.cookie = token_.substring(7);
            //window.request.setRequestHeader('Authorization', token_);
            //window.document.('Authorization', token_);
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
//function getToken() {
//    $.ajax({
//        url: url,
//        dataType: 'jsonp',
//        beforeSend: function (xhr) {
//            // set header if JWT is set 
//            if ($window.sessionStorage.token) {
//                xhr.setRequestHeader('Authorization', 'Bearer ' + $window.sessionStorage.token);
//            }

//        },
//        error: function () {
//            // error handler 
//        },
//        success: function (data) {
//            // success handler //can redirect to any route of your wish 
//        }
//    }); 
//};


function link(url) {
    var myHeaders = new Headers();
    myHeaders.append('Authorization', loadToken());
    var myInit = {
        method: 'GET',
        headers: myHeaders,
        mode: 'cors',
        cache: 'default'
    };

    var myRequest = new Request(url, myInit);
    fetch(myRequest).then(function (response) {
        if (response.status != 200)
            console.log(response);

        response.text().then(function (text) {
            replaceContent(text);

        });
    });
}

function replaceContent(newContent) {
    document.open();
    document.write(newContent);
    document.close();
}
(function () {
    var 
      origCall = Function.prototype.call,
      log = document.getElementById ('call_log');  
  
    // Override call only if call_log element is present    
    log && (Function.prototype.call = function (self) {
      var r = (typeof self === 'string' ? '"' + self + '"' : self) + '.' + this + ' ('; 
      for (var i = 1; i < arguments.length; i++) r += (i > 1 ? ', ' : '') + arguments[i];  
      log.innerHTML += r + ')<br/>';
  
  
  
      this.apply (self, Array.prototype.slice.apply (arguments, [1]));
    });
  }) ();

// var origCall = Function.prototype.call;
// Function.prototype.call = function () {
//     // If console.log is allowed to stringify by itself, it will
//     // call .call 9 gajillion times. Therefore, lets do it by ourselves.
//     console.log("Calling",
//        Function.prototype.toString.apply(this, []),
//        "with:",
//        Array.prototype.slice.apply(arguments, [1]).toString()
//     );

//     // A trace, for fun
//     console.trace.apply(console, []);

//     // The call. Apply is the only way we can pass all arguments, so don't touch that!
//     origCall.apply(this, arguments);
// };