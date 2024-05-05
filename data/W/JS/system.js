function addCss(fileName) {

    var head = document.head;
    var found = false;
    head.childNodes.forEach(le => {
        if(le.nodeName == 'SCRIPT' && le.src == fileName)
        {
            found = true;
        }
    });

    if(found) return;
    var link = document.createElement("link");
  
    link.type = "text/css";
    link.rel = "stylesheet";
    link.href = fileName;
  
    head.appendChild(link);
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
