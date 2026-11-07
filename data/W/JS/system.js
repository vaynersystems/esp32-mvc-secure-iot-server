function addCss(fileName) {

    var request = new XMLHttpRequest();
    var head = document.head;
    var found = false;
    head.childNodes.forEach(le => {
        if(le.nodeName == 'LINK' && le.href == fileName)
        {
            found = true;
        }
    });

    if(found) return;
    var script = document.createElement("style");
  
    script.type = "text/css";
    script.rel = "stylesheet";
    
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = fileName;
    request.open("GET", url, true);
    request.setRequestHeader("Content-type", "text/css");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {

            hideWait('page');
            if (request.status == 401) {
                //showModal('<p class="error">' + request.statusText + '</p>', 'Unauthorized');                
                return;
            }
            var response = request.responseText;
            if(request.status == 200){
                script.innerHTML = response;
                head.appendChild(script);
            }
                            
        }
    }
    request.send();
    showWait('page');

  
    
}  

function decodeUTF8(data) {
    if (typeof data === "string") {
      const utf8 = new Uint8Array(
        Array.prototype.map.call(data, (c) => c.charCodeAt(0))
      );
      return new TextDecoder("utf-8").decode(utf8);
    }
  
    if (Array.isArray(data)) {
      return data.map(decodeUTF8);
    }
  
    if (typeof data === "object") {
      const obj = {};
      Object.entries(data).forEach(([key, value]) => {
        obj[key] = decodeUTF8(value);
      });
      return obj;
    }
  
    return data;
}
  
  


function reset(check){
    var request = new XMLHttpRequest();
    const skipCheck = check !== undefined && check == true ;

    if(!skipCheck){
        showModal('Device Restart ','Are you sure you want to restart the device', [{text:'No',action: () => { closeModal();} }, {text:'Yes', action: () => {
            resetCall();closeModal();
            // Same as the restart after a save: spinner, then reload once the device answers.
            showWait();
            waitForDeviceRestart(120000, 5000);
        }}]);
    }
        
    else
        resetCall();
}

function resetCall(){
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'ResetDevice'
    // Its own request: reusing the page's shared one ran the previous handler (e.g. the
    // settings save) when the restarting device dropped the connection, which reported
    // "An unknown error occured while saving". A dropped connection here is expected.
    const resetRequest = new XMLHttpRequest();
    resetRequest.onreadystatechange = function () {
        if (resetRequest.readyState == resetRequest.DONE)
            console.log('Device restarting (status ' + resetRequest.status + ')');
    };
    resetRequest.open("POST", url, true);
    resetRequest.send(null);
}


// After a restart: poll until the device answers, then reload the page. The device needs
// roughly 10-35 s to boot and join Wi-Fi. Each attempt has its own timeout, so a request to a
// device that is still down cannot hang the loop. Gives up after maxWaitMs with a message
// (e.g. the device came up on another network or address).
function waitForDeviceRestart(maxWaitMs = 120000, firstDelayMs = 5000){
    const started = Date.now();
    const attempt = () => {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 4000);
        fetch('/login', { cache: 'no-store', signal: controller.signal })
        .then(() => {
            clearTimeout(timer);
            hideWait();
            window.location.reload();
        })
        .catch(() => {
            clearTimeout(timer);
            if (Date.now() - started > maxWaitMs) {
                hideWait();
                showModal('Device restarted',
                    '<p>The device has not answered at this address yet. If it joined a different network, ' +
                    'reconnect to that network and open the device at its new address.</p>');
                return;
            }
            setTimeout(attempt, 3000);
        });
    };
    setTimeout(attempt, firstDelayMs);
}
