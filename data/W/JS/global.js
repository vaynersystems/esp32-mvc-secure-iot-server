function showLoading() {
    var l = document.getElementById('page-wait');
    if (l !== null) l.style.visibility = 'visible';
}
function hideLoading() {
    var l = document.getElementById('page-wait');
    if (l !== null) l.style.visibility = 'hidden';
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
            button.addEventListener('click',()=>{ b.action();})
            a.appendChild(button);
        }    
    }
    
    
}

function closeModal(){
    var l = document.getElementById('system-modal');
    if (l !== null) l.style.display = 'none';
}