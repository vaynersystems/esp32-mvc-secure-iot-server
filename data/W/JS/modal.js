function showLoading() {
    const waitComponent =_generateWaitComponent();   
    if (waitComponent !== null && waitComponent !== undefined) 
        waitComponent.style.visibility = 'visible';
}
function hideLoading() {
    _destroyWaitComponent();
}
function showWait(which){
    if(which === undefined || which === null || which === ''){
        const waitElement = document.createElement('wait');
        waitElement.id = 'page-wait';
        waitElement.addEventListener('close', () => { waitElement.parent.removeChild(waitElement)});
        document.body.appendChild(waitElement);
        return waitElement;
    }

    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== null && waitElement !== null) waitElement.style.display = 'block';
}

function hideWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== undefined && waitElement !== null ) waitElement.style.display = 'none'; //hide wait indicator
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