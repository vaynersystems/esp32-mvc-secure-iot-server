
function setControllers(controllerText){

    if(controllerText[0] !== '[') return;
    var controllers = JSON.parse(controllerText);

    controllerListElement = document.getElementsByClassName('home-container');

    if(controllerListElement === undefined) return;
    controllerListElement = controllerListElement[0];
    //group controllers by controller-group
    var grouped = Object.groupBy(controllers, ({group}) => group);
    for(var groupName of Object.keys(grouped)){
        var group = grouped[groupName];
        var groupElement = document.createElement('div');
        groupElement.className = 'box';
        var groupHeaderElement = document.createElement('div');
        groupHeaderElement.className = 'header';
        groupHeaderElement.innerHTML = groupName;
        groupElement.appendChild(groupHeaderElement);

        var controllersContainerElement = document.createElement('div');
        controllersContainerElement.className = "column sub-header";
        for(var controller of group){
            var controllerElement = document.createElement('a');
            controllerElement.href = controller.controller;
            controllerElement.innerHTML = controller.name;


            controllersContainerElement.appendChild(controllerElement);
        }
       
        groupElement.appendChild(controllersContainerElement);
        controllerListElement.appendChild(groupElement);
    }



    
    // if(controllers !== undefined && controllers.length > 0){
    //     controllers.forEach((element) => {
    //         link = Object.keys(element)[0] + '/' + Object.values(element)[0];
    //         if(link[0] !='/') link = '/' + link;
    //         cLink = document.createElement('div');
    //         cLinkAnchor = document.createElement('a');
    //         cLink.appendChild(cLinkAnchor);
    //         cLinkAnchor.href = link;
    //         cLinkAnchor.textContent = 'Controller: ' + Object.keys(element)[0] + ' Action: ' + Object.values(element)[0];
    //         controllerListElement.appendChild(cLink);
    // });
    // }
}