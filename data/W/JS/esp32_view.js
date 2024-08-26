var viewModel;


function openView(containerElementName, name, viewModelUri, items, itemActions, viewActions){
    fetch(viewModelUri)
    .then(response => response.json())
    .then(viewModelDefinition => {
        var viewModelItems = populateViewItems(viewModelDefinition, items);
        viewModel = {
            Name: name,
            Errors: [],
            Items: viewModelItems,
            Container: containerElementName,
            Actions: itemActions === undefined || itemActions === null ? [] : itemActions,
            ViewActions: viewActions === undefined || viewActions === null ? [] : viewActions,
        };
        var viewPage = showListView(viewModel,);
    })
    .catch(error => console.error(error));


    
}

//shows the modal dialog using the model provided
function showListView(viewModel, width){


    // if(viewModel === undefined)
    //     viewModel = createViewModel(undefined);
    if(viewModel.Container === undefined){
        showModal('Error showing list', 'An error occured showing list view. The container specified [' + viewModel.Container + '] was not found.');
        return;
    }
    var container = document.getElementById(viewModel.Container);
    container.innerHTML = '';

    _drawViewHeader(container, viewModel.Name);
    _drawViewTable(container, viewModel.Items, viewModel.Actions);
    _drawViewActions(container, viewModel.ViewActions)
    
    
    var a = document.getElementById('system-modal-actions');
    if (a !== null){
        a.innerHTML = '';
        if(modalModel.Actions === undefined || modalModel.Actions.length <= 0){
            modalModel.Actions = [{text:'Ok', action: ()=>{ closeModal();}}]; //default ok
        }

        for(var b of modalModel.Actions){
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

    return container;
}
function _drawViewHeader(container, name){
    var headerElement = document.createElement('h2');
    headerElement.className = 'section-header';
    headerElement.innerHTML = name;
    container.appendChild(headerElement);
}

function _drawViewTable(container, items, itemActions){
    var tableContainerElement = document.createElement('div');
    tableContainerElement.className = 'section-content';
    var tableElement = document.createElement('table');
    _drawViewTableHeader(tableElement, items, itemActions !== undefined && itemActions !== null);
    _drawViewtableItems(tableElement,items, itemActions);
    tableContainerElement.appendChild(tableElement);
    container.appendChild(tableContainerElement);
}

function _drawViewTableHeader(container, items, hasActions){
    var tableHeaderElement = document.createElement('tr');
    tableHeaderElement.className = 'table-header';
    for(var item of items[0]){
        var itemHeaderElement = document.createElement('th');
        itemHeaderElement.className = 'table-header-item';
        itemHeaderElement.innerHTML = item.Name;
        tableHeaderElement.appendChild(itemHeaderElement);
    }   
    if(hasActions){
        var itemHeaderElement = document.createElement('th');
        itemHeaderElement.className = 'table-header-item';
        itemHeaderElement.innerHTML = 'Actions';
        tableHeaderElement.appendChild(itemHeaderElement);
    }
    container.appendChild(tableHeaderElement);
}

function _drawViewtableItems(container, items, itemActions){    

    for(var item of items){
        var tableItemElement = document.createElement('tr');
        tableItemElement.className = 'table-item';
        for(var modelField of item){
            //console.log(modelField);
            _drawTableField(tableItemElement, modelField);
                
        }
        if(itemActions !== undefined && itemActions !== null){
            var actionsElement = document.createElement('tr');
            actionsElement.className = 'table-item-actions';
            for(var action of itemActions){
                var actionElement = document.createElement('button');
                actionElement.className = 'table-action';
                actionElement.innerHTML = action.text;
                actionElement.record = item
                actionElement.addEventListener('click', action.action);
                actionsElement.appendChild(actionElement);
            }
            tableItemElement.appendChild(actionsElement);
        }

        container.appendChild(tableItemElement);
    }
    
    
}

function _drawTableField(container, modelField){
    
    if(modelField.Readonly){
        var fieldValueElement = document.createElement('td');
        fieldValueElement.innerHTML = modelField.Value ?? '';
        fieldValueElement.id = 'field-value-' + modelField.Name;
        container.appendChild(fieldValueElement);
    }
    // else{
    //     switch(modelField.Type){
    //         case 'Number':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'text';
    //             fieldInputElement.value = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'Text':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'text';
    //             fieldInputElement.value = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'Password':
    //         case 'password':
    //         case 'Secret':
    //         case 'secret':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'password';
    //             fieldInputElement.value = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'Boolean':
    //         case 'boolean':
    //         case 'Bool':
    //         case 'bool':
    //         case 'Bit':
    //         case 'bit':
    //         case 'check':
    //         case 'checkbox':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'checkbox';
    //             fieldInputElement.checked = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.checked);
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'Date':
    //         case 'date':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'date';
    //             fieldInputElement.value = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;                
    //         case 'File':
    //         case 'file':
    //             var fieldInputElement = document.createElement('input');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.type = 'file';
    //             fieldInputElement.value = modelField.Value ?? '';
    //             fieldInputElement.addEventListener('change',(ev) => {
    //                 modelField.Value = fieldInputElement.value;
    //                 modelField.Files = fieldInputElement.files;
    //             });
    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'Lookup':
    //             var fieldInputElement = document.createElement('select');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                

    //             if(modelField.Data != undefined)
    //                 for(var option of modelField.Data){
    //                     // TODO: implement unique check
    //                     //if unique, filter out options used already
    //                     var optionElement = document.createElement('option');
    //                     optionElement.value = option.value;
    //                     optionElement.text = option.name;
    //                     fieldInputElement.appendChild(optionElement);
    //                 }

    //             if(modelField.Value !== undefined)
    //                 fieldInputElement.value = modelField.Value;

    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //         case 'LookupGrouped':
    //             var fieldInputElement = document.createElement('select');
    //             fieldInputElement.id = 'field-value-' + modelField.Name;
    //             fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                
    //             if(modelField.Data != undefined){
    //                 var grouped = Object.groupBy(modelField.Data, ({group}) => group); 
    //                 for(var groupName of Object.keys(grouped)){
    //                     var group = grouped[groupName];
    //                     var groupElement = document.createElement('optgroup');
    //                     groupElement.label = groupName;
                        
    //                     for(var option of group){
    //                         var optionElement = document.createElement('option');
    //                         optionElement.value = option.value;
    //                         optionElement.text = option.name;
    //                         groupElement.appendChild(optionElement);
    //                     }
    //                     if(groupElement !== undefined)
    //                     fieldInputElement.appendChild(groupElement);
    //                 }
                    
    //             }

    //             if(modelField.Value !== undefined)
    //                 fieldInputElement.value = modelField.Value;

    //             fieldContainerElement.appendChild(fieldInputElement);
    //             break;
    //     }    
    // }
         
    
    //container.appendChild(fieldContainerElement);
}

function _drawViewActions(container, actions){
    if(actions === undefined || actions === null)
        return;

    var actionsElement = document.createElement('div');
    actionsElement.className = 'table-actions';
    for(var action of actions){
        var actionElement = document.createElement('button');
        actionElement.className = 'table-action';
        actionElement.innerHTML = action.text;
        actionElement.addEventListener('click', action.action);
        actionsElement.appendChild(actionElement);
    }
    container.appendChild(actionsElement);
}

/*
* @returns viewModelItems array
*/
function populateViewItems(sourceModel, items, optional){
    var viewModelItems = [];
    if(sourceModel === undefined) return;
    for(var record of items){
        var destinationViewItem = window.structuredClone(sourceModel);
        for(property in record){
            var modelProperty = record[property];
            if( typeof modelProperty === 'object' ){
                //enumerate children
                for(childProperty in modelProperty){
                    var viewModelProperty = destinationViewItem.find(field => field.Source == property + "." + childProperty);  
                    if(viewModelProperty === undefined) continue; //property not used                   
                        viewModelProperty.Value = record[property][childProperty];                    
                }
            } else{
                var viewModelProperty = destinationViewItem.find(field => field.Source == property);
                if(viewModelProperty === undefined) continue; //property not used             
                viewModelProperty.Value = record[property]
            }
            
        }
        //populate optional
        if(optional !== undefined && optional !== null)
        {
            for(var option of optional){
                var viewModelField = destinationViewItem.find(f => f.Source == option.Source);
                if(viewModelField !== undefined)
                    viewModelField[option.Field] = option.Value;
            };
        }
        //leave source
        destinationViewItem.Record = record;
        viewModelItems.push(destinationViewItem);
    }
    return viewModelItems;
}