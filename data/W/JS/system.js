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

    if(!skipCheck)
        showModal('Are you sure you want to restart the device','Device Restart ', [{text:'No',action: () => { closeModal();} }, {text:'Yes', action: () => {resetCall();closeModal();}}]);
    else
        resetCall();
}

function resetCall(){
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'ResetDevice'
    request.open("POST", url, true);
    request.send(null);
}
