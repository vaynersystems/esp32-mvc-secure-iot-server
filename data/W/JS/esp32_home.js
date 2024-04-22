
function setControllers(controllerText){

    var controllers = JSON.parse(controllerText);
    controllerListElement = document.getElementById('controller-list');
    if(controllers !== undefined && controllers.length > 0){
        controllers.forEach((element) => {
            link = Object.keys(element)[0] + '/' + Object.values(element)[0];
            cLink = document.createElement('div');
            cLinkAnchor = document.createElement('a');
            cLink.appendChild(cLinkAnchor);
            cLinkAnchor.href = link;
            cLinkAnchor.textContent = link;
            controllerListElement.appendChild(cLink);
    });
    }
}