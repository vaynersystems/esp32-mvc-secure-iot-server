function showLoading() {
    const waitComponent =_generateWaitComponent();   
    if (waitComponent !== null && waitComponent !== undefined) 
        waitComponent.style.visibility = 'visible';
}
function hideLoading() {
    _destroyWaitComponent();
}

// function showModalComponent(component, header, actions, width){
//     const modalComponent = _generateModalComponent(width);
//     //var m = document.getElementById('system-modal');
//     //if (modalComponent !== null) m.style.display = 'block';

//     var t = document.getElementById('system-modal-text');
//     if (t !== null) {
//         //t.innerHTML = component.innerHTML;        
//         t.innerHTML = '';
//         t.appendChild(component);
//     }

//     var h = document.getElementById('system-modal-header');
//     if (h !== null) h.innerHTML = header === undefined ? 'ESP32 MODAL' : header;

//     var a = document.getElementById('system-modal-actions');
//     if (a !== null){
//         a.innerHTML = '';
//         if(actions === undefined || actions.length <= 0){
//             actions = [{text:'Ok', action: ()=>{ closeModal();}}]; //default ok
//         }

//         for(var b of actions){
//             const button = document.createElement('button');
//             button.textContent = b.text;            
//             button.addEventListener('click', b.action);
//             a.appendChild(button);
//         }    
//     }
//     window.addEventListener('keydown', function(key) {
//         if(key.code == 'Escape')
//             closeModal();
//     })
// }

// function showModal(text, header, actions, width){
//     const modalComponent = _generateModalComponent(width);
    
//     var t = document.getElementById('system-modal-text');
//     if (t !== null) t.innerHTML = text;

//     var h = document.getElementById('system-modal-header');
//     if (h !== null) h.innerHTML = header === undefined ? 'ESP32 MODAL' : header;

//     var a = document.getElementById('system-modal-actions');
//     if (a !== null){
//         a.innerHTML = '';
//         if(actions === undefined || actions.length <= 0){
//             actions = [{text:'Ok', action: ()=>{ closeModal();}}]; //default ok
//         }

//         for(var b of actions){
//             const button = document.createElement('button');
//             button.textContent = b.text;            
//             button.addEventListener('click', b.action);
//             a.appendChild(button);
//         }    
//     }   

//     window.addEventListener('keydown', function(key) {
//         if(key.code == 'Escape')
//             closeModal();
//     })    
// }

// function closeModal(){
//     var l = document.getElementById('system-modal');
//     if (l !== null) _destroyModalComponent();
// }

function showWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== null && waitElement !== null) waitElement.style.display = 'block';
}

function hideWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== undefined && waitElement !== null ) waitElement.style.display = 'none'; //hide wait indicator
}


// /* ******************************** */
// //generates the HTML component drawing the modal window and adds it to the document
// function _generateModalComponent(width){
//     const html = `
//     <div id="system-modal">        
//         <div id="system-modal-content">
//             <div class="row">
//                 <div id="system-modal-header"></div>
//                 <div id="system-modal-close" onclick="closeModal();">X</div>                
//             </div>
//             <div id="system-modal-text"></div>
//             <div id="system-modal-actions">
//                 <button onclick="closeModal()">Ok</button>
//             </div>
//         </div>        
//     </div>`;
//     const modalComponent = document.createElement('div');
//     modalComponent.innerHTML = html;
//     modalComponent.className = "system-modal-container"
//     document.body.appendChild(modalComponent);

//     const modalContentComponent = document.getElementById('system-modal-content');
//     if(modalContentComponent !== null && width !== undefined) {
//         modalContentComponent.style.minWidth = width;
//         modalContentComponent.style.maxWidth = width;
//         modalContentComponent.style.width = width;
//     }
//     return modalComponent
// }

//TODO: if having multiple modal windows, pass an id, maintain a list of modals
function _destroyModalComponent(){
    const m = document.getElementById('system-modal');
    if(m !== null)
        m.parentElement.removeChild(m);
}

function _generateWaitComponent(){
    _destroyWaitComponent();
    const waitComponent = document.createElement('div');
    waitComponent.innerHTML = '<div id="page-wait"></div>';
    document.body.appendChild(waitComponent);
    return waitComponent;
}

function _destroyWaitComponent(){
    const m = document.getElementById('page-wait');
    if(m !== null)
        m.parentElement.removeChild(m);
}