function showLoading() {
    var l = document.getElementById('page-wait');
    if (l !== null) l.style.visibility = 'visible';
}
function hideLoading() {
    var l = document.getElementById('page-wait');
    if (l !== null) l.style.visibility = 'hidden';
}

function showModalComponent(component, header, actions){
    var m = document.getElementById('system-modal');
    if (m !== null) m.style.display = 'block';

    var t = document.getElementById('system-modal-text');
    if (t !== null) {
        //t.innerHTML = component.innerHTML;        
        t.innerHTML = '';
        t.appendChild(component);
    }

    var h = document.getElementById('system-modal-header');
    if (h !== null) h.innerHTML = header === undefined ? 'ESP32 MODAL' : header;

    var a = document.getElementById('system-modal-actions');
    if (a !== null){
        a.innerHTML = '';
        if(actions === undefined || actions.length <= 0){
            actions = [{text:'Ok', action: ()=>{ closeModal();}}]; //default ok
        }

        for(var b of actions){
            const button = document.createElement('button');
            button.textContent = b.text;            
            button.addEventListener('click', b.action);
            a.appendChild(button);
        }    
    }
    window.addEventListener('keydown', function(key) {
        if(key.code == 'Escape')
            closeModal();
    })
}

function showModal(text, header, actions){
    var m = document.getElementById('system-modal');
    if (m !== null) m.style.display = 'block';

    var t = document.getElementById('system-modal-text');
    if (t !== null) t.innerHTML = text;

    var h = document.getElementById('system-modal-header');
    if (h !== null) h.innerHTML = header === undefined ? 'ESP32 MODAL' : header;

    var a = document.getElementById('system-modal-actions');
    if (a !== null){
        a.innerHTML = '';
        if(actions === undefined || actions.length <= 0){
            actions = [{text:'Ok', action: ()=>{ closeModal();}}]; //default ok
        }

        for(var b of actions){
            const button = document.createElement('button');
            button.textContent = b.text;            
            button.addEventListener('click', b.action);
            a.appendChild(button);
        }    
    }   
    
}

function closeModal(){
    var l = document.getElementById('system-modal');
    if (l !== null) l.style.display = 'none';
}

function showWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== null && waitElement !== null) waitElement.style.display = 'block';
}

function hideWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== undefined && waitElement !== null ) waitElement.style.display = 'none'; //hide wait indicator
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
