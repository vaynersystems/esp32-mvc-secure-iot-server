
    /*****  MODAL CODE, goes into seperate and reusable component  ******/
    var modalModel;
    
    /**
     *  Opens Modal view with view model and model provided
     *  Can call save or cancell callback functions when completed
     * @param {text} name - Form Name,
     * @param {text} viewModelUri - path where model json is located
     * @param {object} model - your object model or DTO
     * @param {array} optional - list of optional parameters in the form {Source: 'Model Path of data point', Field: 'View Model field to bind', Value:'Value to bind'}
     * @param {function} saveCallback - function to call after form is saved
     * @param {function} cancelCallback - function to call after form is cancelled
     * @param {function} validationCallback - function to before form is saved
     */
    function openModal(name, viewModelUri, model, optional, saveCallback, cancellCallback, validationCallback){
        fetch(viewModelUri)
            .then(response => response.json())
            .then(viewModel => {
                openModalWithModel(name,viewModel,model,optional, saveCallback, cancellCallback, validationCallback);
            })
            .catch(error => console.error(error));
    }

    function openModalWithModel(name, viewModel, model, optional, saveCallback, cancellCallback, validationCallback){
        //populate deviceModel fields -- generic for all objects
        populateViewModel(model, viewModel, optional);

        modalModel = {
            Name: name, /* string */
            Errors: [],
            Items: viewModel,
            Actions: [
                { text:'Cancel',action: () => { confirmCancel(cancellCallback);}},
                { text: 'Save', action:  () => { if(validate(viewModel,validationCallback)){saveModel(model);closeModal(saveCallback)}}}
            ],                
        };

        var modalWindow = showModalModel(modalModel);  
    }

    function confirmCancel(cancelCallback){
        const modalComponent = _generateModalComponent(); 
        showModalComponent(
            modalComponent,
            'Confirm Cancel', 
            'Are you sure you want to cancel', 
            [{text:'No', action: () => {_destroyModalComponent(modalComponent);return false;}}, {text:'Yes', action: () => {_destroyModalComponent(modalComponent); closeModal(cancelCallback);}}]
        )
    }
    function validate(viewModel, validationCallback){
        var formErrors = [];
        var errorsElement = document.getElementById('system-modal-errors');
        errorsElement.innerHTML = '';

        for(property of viewModel){
            if(property.Validation !== undefined){
                for(var rule of property.Validation){
                    var errorsFoundInField = false;
                    switch(rule.Type){
                        case "Required":
                            if(property.Value === undefined || property.Value === null || property.Value.length == 0){
                                formErrors.push(property.Name + " is required.");                            
                                errorsFoundInField = true;
                            }
                            break;
                        case "MinLength":
                            if(property.Value === undefined || property.Value === null || property.Value.length < rule.Value)
                            {
                                formErrors.push(property.Name + " requires at least " + rule.Value + " characters.");  
                                errorsFoundInField = true;
                            }
                        break;
                        case "MaxLength":
                            if(property.Value === undefined || property.Value === null || property.Value.length > rule.Value)
                            {
                                formErrors.push(property.Name + " cannot exceed " + rule.Value + " characters.");  
                                errorsFoundInField = true;
                            }
                        break;
                        case "Min":
                            if(property.Value === undefined || property.Value === null || property.Value < rule.Value)
                            {
                                formErrors.push(property.Name + " must be at least " + rule.Value + ".");  
                                errorsFoundInField = true;
                            }
                        break;
                        case "Max":
                            if(property.Value === undefined || property.Value === null || property.Value < rule.Value)
                            {
                                formErrors.push(property.Name + "can be at most" + rule.Value + ".");  
                                errorsFoundInField = true;
                            }
                        break;
                        default:
                            break;
                    }
                    if(errorsFoundInField) break; //if field failed one test, no need to report other failures
                }
            }            
        }

        if(validationCallback !== undefined){
            var validationErrors = validationCallback(viewModel);
            if(validationErrors !== undefined && Array.isArray(validationErrors)){
                formErrors = [...validationErrors];
            }
        }
        if(formErrors.length > 0){
            
            for(var error of formErrors){
                var errorElement = document.createElement('div');
                errorElement.className = "error";                
                errorElement.innerHTML = error;
                errorsElement.appendChild(errorElement);
            }
            return false;
        }
        return true;
    }

    function showModal(title, content, actions){
        const modalComponent = _generateModalComponent(); 
        showModalComponent(modalComponent,title, content, actions);
    }

    function showModalComponent(modalComponent,title, content, actions){
        var t = modalComponent.querySelector('#system-modal-text');
        if (t !== null) {   
            t.innerHTML = content;
        }

        var h = modalComponent.querySelector('#system-modal-header');
        if (h !== null) h.innerHTML = title === undefined ? 'ESP32 MODAL' : title;

        var a = modalComponent.querySelector('#system-modal-actions');
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

        return modalComponent;
    }
    //shows the modal dialog using the model provided
    function showModalModel(viewModel, width){

        //close if any are open
        closeModal();

        if(viewModel === undefined)
            viewModel = createViewModel(undefined);


        const modalComponent = _generateModalComponent(width);        
        var t = modalComponent.querySelector('#system-modal-text');
        //var t = document.getElementById('system-modal-text');
        if (t !== null) {   
            t.innerHTML = '';
            _drawModalFields(t, viewModel.Items);
        }

        var h = document.getElementById('system-modal-header');
        if (h !== null) h.innerHTML = viewModel.Name === undefined ? 'ESP32 MODAL' : viewModel.Name;

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

        return modalComponent;
    }

    function _drawModalFields(container, model){    
        for(var modelField of model){
            //console.log(modelField);
            _drawModalField(container, modelField);
                
        }
    }

    function _drawModalField(container, modelField){
        var fieldContainerElement = document.createElement('div');
        fieldContainerElement.className = 'grid-2';
        fieldContainerElement.id = 'field-container-' + modelField.Name;
        
        var fieldNameElement = document.createElement('span');
        fieldNameElement.innerHTML = modelField.Name;
        fieldContainerElement.appendChild(fieldNameElement);

        if(modelField.Readonly){
            var fieldValueElement = document.createElement('span');
            fieldValueElement.innerHTML = modelField.Value ?? '';
            fieldValueElement.id = 'field-value-' + modelField.Name;
            fieldContainerElement.appendChild(fieldValueElement);
        }
        else{
            switch(modelField.Type){
                case 'Number':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'text';
                    fieldInputElement.value = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'Text':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'text';
                    fieldInputElement.value = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'Password':
                case 'password':
                case 'Secret':
                case 'secret':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'password';
                    fieldInputElement.value = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'Boolean':
                case 'boolean':
                case 'Bool':
                case 'bool':
                case 'Bit':
                case 'bit':
                case 'check':
                case 'checkbox':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'checkbox';
                    fieldInputElement.checked = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.checked);
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'Date':
                case 'date':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'date';
                    fieldInputElement.value = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;                
                case 'File':
                case 'file':
                    var fieldInputElement = document.createElement('input');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.type = 'file';
                    fieldInputElement.value = modelField.Value ?? '';
                    fieldInputElement.addEventListener('change',(ev) => {
                        modelField.Value = fieldInputElement.value;
                        modelField.Files = fieldInputElement.files;
                    });
                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'Lookup':
                    var fieldInputElement = document.createElement('select');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    

                    if(modelField.Data != undefined)
                        for(var option of modelField.Data){
                            // TODO: implement unique check
                            //if unique, filter out options used already
                            var optionElement = document.createElement('option');
                            optionElement.value = option.value;
                            optionElement.text = option.name;
                            fieldInputElement.appendChild(optionElement);
                        }

                    if(modelField.Value !== undefined)
                        fieldInputElement.value = modelField.Value;

                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
                case 'LookupGrouped':
                    var fieldInputElement = document.createElement('select');
                    fieldInputElement.id = 'field-value-' + modelField.Name;
                    fieldInputElement.addEventListener('change',(ev) => modelField.Value = fieldInputElement.value);
                    
                    if(modelField.Data != undefined){
                        var grouped = Object.groupBy(modelField.Data, ({group}) => group); 
                        for(var groupName of Object.keys(grouped)){
                            var group = grouped[groupName];
                            var groupElement = document.createElement('optgroup');
                            groupElement.label = groupName;
                            
                            for(var option of group){
                                var optionElement = document.createElement('option');
                                optionElement.value = option.value;
                                optionElement.text = option.name;
                                groupElement.appendChild(optionElement);
                            }
                            if(groupElement !== undefined)
                            fieldInputElement.appendChild(groupElement);
                        }
                        
                    }

                    if(modelField.Value !== undefined)
                        fieldInputElement.value = modelField.Value;

                    fieldContainerElement.appendChild(fieldInputElement);
                    break;
            }    
        }

        fieldContainerElement.checkConditions = function (){
            if(modelField.Condition1 !== undefined){
                checkCondition(fieldContainerElement, 'condition1Met', modelField.Condition1, modelField.ConditionValue1);
            }
            if(modelField.Condition2 !== undefined){
                checkCondition(fieldContainerElement, 'condition2Met', modelField.Condition2, modelField.ConditionValue2);
            }

            fieldContainerElement.style.display = fieldContainerElement.condition1Met && fieldContainerElement.condition2Met ? 'grid' : 'none';
        }
        fieldContainerElement.condition1Met = true;
        fieldContainerElement.condition2Met = true;
        
            
        if(modelField.Condition1 !== undefined){
            initConditionCheck(container, fieldContainerElement, 'condition1Met', modelField.Condition1, modelField.ConditionValue1);
        }
        if(modelField.Condition2 !== undefined){
            initConditionCheck(container, fieldContainerElement, 'condition2Met', modelField.Condition2, modelField.ConditionValue2);
        }
        
        fieldContainerElement.style.display = fieldContainerElement.condition1Met && fieldContainerElement.condition2Met ? 'grid' : 'none';
              
        
        container.appendChild(fieldContainerElement);
    }

 
    function initConditionCheck(container, fieldContainerElement, conditionName, condition, value){
        var conditionMet = false;
        var fieldName = condition.includes('.') ? condition.substring(0,condition.indexOf(".")) : condition;

        //get field representing condition. bind event change to check condition
        var modelConditionalElement = container.querySelector('[id="field-value-' +fieldName + '"]');
        if(modelConditionalElement !== null){
            fieldContainerElement.checkConditions();

            //add event
            modelConditionalElement.addEventListener('change', (visualElement) =>{
                fieldContainerElement.checkConditions();
            });
        }
    }

    function checkCondition(fieldContainerElement, conditionName, condition, value){

        var conditionMet = false;
        var fieldName = condition;
        var fieldCompareName = fieldName;
        if(condition.includes(".")){
            fieldName = condition.substring(0,condition.indexOf("."));
            //field we want to use for comparison
            fieldCompareName = condition.substring(condition.indexOf(".") + 1);
            var fieldValue = modalModel.Items.find(i => i.Name == fieldName);
            var fieldData = fieldValue.Value === undefined ? '': fieldValue?.Data.find(d => d['value'] == fieldValue.Value);

            conditionMet = fieldData[fieldCompareName] == value;
            fieldContainerElement[conditionName] = conditionMet;
        }  else{
            conditionMet = modalModel.Items.find(i => i.Name == fieldCompareName)?.Value == value;
            fieldContainerElement[conditionName] = conditionMet;
        }
        
        switch(fieldContainerElement.type){
            case 'checkbox':
                conditionMet = fieldContainerElement.checked == value;
                break;
            case 'text':
            case 'number':
            case 'date':
            case 'select':
                conditionMet = fieldContainerElement.value == value;
                break;
        }
        
        fieldContainerElement[conditionName] = conditionMet;
        return conditionMet;
    }

    function createViewModel(){
        
        var viewModel = {
        Name: ' *** ', /* string */
        Items: [],         
        Actions: [
            { text:'Cancel',action: () => { closeModal();}},
            { text: 'Save', action:  () => { saveModel(device);closeModal();}}
        ]};
        return viewModel;
        
    }

    function populateViewModel(sourceModel, viewModel, optional){
        if(sourceModel === undefined) return;

        for(property in sourceModel){
            var modelProperty = sourceModel[property];
            if( typeof modelProperty === 'object' ){
                //enumerate children
                for(childProperty in modelProperty){
                    var viewModelProperty = viewModel.find(field => field.Source == property + "." + childProperty);  
                    if(viewModelProperty === undefined) continue; //property not used                   
                        viewModelProperty.Value = sourceModel[property][childProperty];                    
                }
            } else{
                var viewModelProperty = viewModel.find(field => field.Source == property);
                if(viewModelProperty === undefined) continue; //property not used             
                viewModelProperty.Value = sourceModel[property]
            }
            
        }
        //populate optional
        if(optional === undefined || optional === null)
            return;
        for(var option of optional){
            var viewModelField = viewModel.find(f => f.Source == option.Source);
            if(viewModelField !== undefined)
                viewModelField[option.Field] = option.Value;
        };
    }

    var returnModel = {};
    function saveModel(sourceModel){
        if(sourceModel === undefined || sourceModel === null) sourceModel = {};

        var destField = undefined;
        for(property of modalModel.Items){
            var isFile = false;
            if(property.Type == 'File' || property.Type == 'file'){
                isFile = true;
            }
            if(property.Source.includes('.')){
                var parentName = property.Source.substring(0,property.Source.indexOf('.'));
                var childName = property.Source.substring(property.Source.indexOf('.') + 1);
                if(sourceModel[parentName] === undefined)
                    sourceModel[parentName] = {};
                sourceModel[parentName][childName] = isFile ? property.Files[0] : property.Value;
            }
            else{
                sourceModel[property.Source] = isFile ? property.Files[0] : property.Value;
            }

            //include the file in upload
            
        }
        //console.log('Saving Model ' + modalModel.Name , /* modalModel, */ sourceModel);
        returnModel = sourceModel;
    }
    



    function closeModal(callbackFn){
        var l = document.getElementById('system-modal');        
        if (l === null) return;
        //console.log('Closing Modal');
        if(callbackFn !== undefined && callbackFn !== null)
        callbackFn(returnModel);
        
        _destroyModalComponent(l);
    }


    /* ******************************** */
    //generates the HTML component drawing the modal window and adds it to the document
    function _generateModalComponent(width){
        const html = `
        <div id="system-modal">        
            <div id="system-modal-content">
                <div class="row">
                    <div id="system-modal-header"></div>
                    <div id="system-modal-close" onclick="closeModal();">X</div>                
                </div>
                <div id="system-modal-text"></div>
                 <div id="system-modal-errors"></div>
                <div id="system-modal-actions">
                    <button onclick="closeModal()">Ok</button>
                </div>
            </div>        
        </div>`;
        const modalComponent = document.createElement('div');
        modalComponent.innerHTML = html;
        modalComponent.className = "system-modal-container"
        document.body.appendChild(modalComponent);

        const modalContentComponent = document.getElementById('system-modal-content');
        if(modalContentComponent !== null && width !== undefined) {
            modalContentComponent.style.minWidth = width;
            modalContentComponent.style.maxWidth = width;
            modalContentComponent.style.width = width;
        }
        modalComponent.dispatchEvent(new Event("close"));
        return modalComponent;
    }

    //TODO: if having multiple modal windows, pass an id, maintain a list of modals
    function _destroyModalComponent(modal){
        //const m = document.getElementById('system-modal');
        if(modal !== null)
            modal.parentElement.removeChild(modal);
    }
