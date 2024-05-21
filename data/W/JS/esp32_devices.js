
/* DEVICES */
deviceTypes = [
    {name: 'Thermometer', description: 'Thermometer', Type: 'input'},
    {name: 'Relay', description: 'Relay', Type: 'output'}
]
class esp32_devices{

    loadDevices(devices){
        const deviceListElement = document.getElementById('device-list');
        if(deviceListElement === null) return;
    
        deviceListElement.innerHTML = '';
        //loadDeviceTypes();
    
        for(const device of devices){
            const rowElement = document.createElement('div');
            const idElement = document.createElement('div');
            const nameElement = document.createElement('div');
            const typeElement = document.createElement('div');
            const pinElement = document.createElement('div');
            const actionsElement = document.createElement('div');
            const editButtonElement = document.createElement('button');
            const deleteButtonElement = document.createElement('button');
    
            idElement.textContent = device.id;
            nameElement.textContent = device.name;
            typeElement.textContent = device.type;
            pinElement.textContent = device.pin;
    
            rowElement.setAttribute('device-name', device.name);
    
            editButtonElement.textContent = 'Edit';
            deleteButtonElement.textContent = 'Delete';
            editButtonElement.addEventListener('click', (event ) => { event.stopPropagation(); this.showDeviceEditor(device); });
            deleteButtonElement.addEventListener('click', (event) =>  {
                event.stopPropagation();
                showModal(
                    'Are you sure you want to delete the <b>' + rowElement.getAttribute('device-name') + '</b> device?',
                    'Confirm Device Removal',
                    [
                        {text:'No', action: () => {closeModal()}}, 
                        {text: 'Yes', action: () => 
                            { 
                                activeConfig.devices = activeConfig.devices.filter(e => e.id == device.id);
                                closeModal();
                                loadDevices(activeConfig.devices);
                            } 
                        }
                    ]
                )
            });
            actionsElement.appendChild(editButtonElement);
            actionsElement.appendChild(deleteButtonElement);
            
    
            rowElement.appendChild(idElement);
            rowElement.appendChild(nameElement);
            rowElement.appendChild(typeElement);
            rowElement.appendChild(pinElement);
            rowElement.appendChild(actionsElement);
    
            rowElement.setAttribute('data', JSON.stringify(device));
            rowElement.className = "grid-row grid";
            rowElement.addEventListener('click', () => this.showDeviceEditor(JSON.parse(rowElement.getAttribute('data'))))
            deviceListElement.appendChild(rowElement);
        }
    }
    
    
    loadDeviceTypes(){
        const deviceList = document.getElementById('editor-device-type');
        if(deviceList === undefined || deviceList === null) return;
    
        for(var device in deviceTypes){
            const opt = document.createElement('opt');
            opt.value = device.name;
            opt.textContent = device.description;
        }
    
    }
    
    //Save device edited in the form
    saveDevice(){
        const modalElement = document.getElementById("system-modal-content");
        const triggerSelector = modalElement.querySelector('#editor-device-use-trigger');
        const toggleMQTTElement = modalElement.querySelector('#editor-device-mqtt-publish');
        
        
        var device = {};
        device.id = modalElement.querySelector('#editor-device-id').innerHTML;
        device.name = modalElement.querySelector('#editor-device-name').value;
        device.type = modalElement.querySelector('#editor-device-type').value;
        device.pin = modalElement.querySelector('#editor-device-pin').value;
        device.signal = modalElement.querySelector('#editor-device-signal').value;
        if(device.type === 'Relay')
            device.duration = modalElement.querySelector('#editor-device-duration').value;
        if(triggerSelector.checked){
        device.trigger = {};    
            device.trigger.active = triggerSelector.checked;
            device.trigger.source = modalElement.querySelector('#editor-device-trigger-source-device').value;
            device.trigger.type = modalElement.querySelector('#editor-device-trigger-type').value;
            device.trigger.value = modalElement.querySelector('#editor-device-trigger-value').value;
            device.trigger.threshold = modalElement.querySelector('#editor-device-trigger-threshold').value;
        }
    
        const isOutputDevice = device.type == 'Relay' || device.type == 'Switch';
        if(device.mqtt === undefined) device.mqtt = {};
        device.mqtt.publish = toggleMQTTElement.checked;
        if(device.mqtt.publish){
            device.mqtt.topic = modalElement.querySelector('#editor-device-mqtt-topic').value;
            if(!isOutputDevice){
                device.mqtt.frequency = modalElement.querySelector('#editor-device-mqtt-frequency').value;
            }        
        }
        console.log('saving device', device);
    
        //update in active config. refresh list
        if(activeConfig.devices === undefined)
            activeConfig.devices = [];
    
        const deviceIdx = activeConfig.devices.findIndex(d => d.id == device.id);
        //var d = activeConfig.devices.find(d => d.id == device.id);
        if(deviceIdx >= 0) // replace
            activeConfig.devices.splice(deviceIdx,1,device);
        else
            activeConfig.devices.push(device);
        
        this.loadDevices(activeConfig.devices);
    
        pendingChanges = true;
    
    }
    showDeviceEditor(device){
        const deviceEditorSourceElement = document.getElementById('device-editor');
        if(deviceEditorSourceElement === null) return;
        const deviceEditorElement = deviceEditorSourceElement.cloneNode(true);
        deviceEditorElement.setAttribute('modal', deviceEditorElement.id);
        //deviceEditorElement.innerHTML = deviceEditorSourceElement.innerHTML;
    
        //let device = activeConfig.devices.find(d => d.id === device.id);
        if(device === undefined){        
            const maxId = activeConfig.devices.length > 0 ? Math.max(...activeConfig.devices.map(d => d.id)) : 0;
            device = {
                id: maxId+1,
                name:'New Device',
                type:'',
                pin:'',
                signal:'',
                trigger: {
                    source: 1,
                    type: '=',
                    value: 150
                },
                mqtt:{
                    publish: false
                }
            }
        }
        if(device.mqtt === undefined)
            device.mqtt = {};
    
        const deviceIdElement = deviceEditorElement.querySelector('#editor-device-id');
        const deviceNameElement = deviceEditorElement.querySelector('#editor-device-name');
        const deviceTypeElement = deviceEditorElement.querySelector('#editor-device-type');
        const devicePinElement = deviceEditorElement.querySelector('#editor-device-pin'); 
        const deviceSignalElement = deviceEditorElement.querySelector('#editor-device-signal'); 
        const deviceDurationElement = deviceEditorElement.querySelector('#editor-device-duration'); 
        
        const deviceTriggerSourceElement = deviceEditorElement.querySelector('#editor-device-trigger-source-device');
        const deviceTriggerTypeElement = deviceEditorElement.querySelector('#editor-device-trigger-type'); 
        const deviceTriggerValueElement = deviceEditorElement.querySelector('#editor-device-trigger-value'); 
        const deviceTriggerThresholdElement = deviceEditorElement.querySelector('#editor-device-trigger-threshold'); 
        
        const triggerPanelElement = deviceEditorElement.querySelector('#editor-device-trigger-panel');
        const triggerContainerElement = deviceEditorElement.querySelector('#editor-device-use-trigger-container');
        const toggleTriggerElement = deviceEditorElement.querySelector('#editor-device-use-trigger');    
        const deviceSignalContainerElement = deviceEditorElement.querySelector('#editor-device-signal-container');
        const deviceDurationContainerElement = deviceEditorElement.querySelector('#editor-device-duration-container');
        const toggleMQTTElement = deviceEditorElement.querySelector('#editor-device-mqtt-publish');
        const mqttFrequencyContainerElement = deviceEditorElement.querySelector('#editor-device-mqtt-frequency-container');
        const mqttPanelElement = deviceEditorElement.querySelector('#editor-device-mqtt-panel');
        const mqttTopicElement = deviceEditorElement.querySelector('#editor-device-mqtt-topic');
        const mqttFrequencyElement = deviceEditorElement.querySelector('#editor-device-mqtt-frequency');
    
        if(deviceIdElement !== null) deviceIdElement.innerHTML =  device.id;
        if(deviceNameElement !== null) deviceNameElement.value = device.name;
        if(deviceTypeElement !== null) {
            if(device.type !== undefined) deviceTypeElement.value = device.type;
            toggleTriggerElement.checked =  device.trigger === undefined ? false : device.trigger.active;
            toggleMQTTElement.checked = device.mqtt.publish === undefined ? false : device.mqtt.publish;
            // if(device.type.length > 0) deviceTypeElement.value = device.type;
            var isOutputDevice = device.type == 'Relay' || device.type == 'Switch';
            triggerContainerElement.style.display = isOutputDevice ? 'block' : 'none'; 
            deviceSignalContainerElement.style.display = isOutputDevice ? '' : 'none'; 
            deviceDurationContainerElement.style.display = device.type == 'Relay' ? 'block' : 'none';
            
            triggerPanelElement.style.display = isOutputDevice && toggleTriggerElement.checked ? 'block' : 'none'; 
            mqttPanelElement.style.display = toggleMQTTElement.checked ? 'block' : 'none';
            mqttFrequencyContainerElement.style.display =  isOutputDevice ? 'none' : 'grid';
    
            deviceTypeElement.addEventListener('change',(event) => {
                var isOutputDevice = event.target.value == 'Relay' || event.target.value == 'Switch';
                triggerContainerElement.style.display = isOutputDevice ? '' : 'none'; 
                deviceSignalContainerElement.style.display = isOutputDevice ? '' : 'none'; 
                deviceDurationContainerElement.style.display = event.target.value == 'Relay' ? 'grid' : 'none';
                triggerPanelElement.style.display = isOutputDevice && toggleTriggerElement.checked ? 'block' : 'none'; 
                mqttFrequencyContainerElement.style.display = isOutputDevice ? 'none' : 'grid';
            })
        }
        if(devicePinElement !== null) {
            devicePinElement.setAttribute('value', device.pin);
            if(device.pin.length > 0) devicePinElement.value = device.pin;
            //filter out any device pins used by other devices
            devicePinElement.child
            for(var child=devicePinElement.firstChild; child!==null; child=child.nextSibling) {
                if(child.tagName !== undefined && child.tagName.toUpperCase() == 'OPTION'){
                    if(activeConfig.devices.findIndex(d => d.id != device.id && d.pin == child.value) >= 0){
                        child.disabled = true;
                        if(child.selected) {
                            if(child.nextSibling !== null) child.nextSibling.selected = true;
                            child.selected = false;
                        }
                    }
                }
            }
            
        }
        if(deviceSignalElement !== null) {
            deviceSignalElement.setAttribute('value', device.signal);
            if(device.signal !== undefined && device.signal.length > 0) deviceSignalElement.value = device.signal;
        }
    
        if(deviceDurationElement !== null && device.type == 'Relay') {
            //deviceDurationElement.setAttribute('value', device.duration);
            if(device.duration !== undefined && device.duration.length > 0) deviceDurationElement.value = device.duration;
        }
    
        if(toggleMQTTElement !== null)
    
            toggleMQTTElement.addEventListener('change', (event) => {
                isOutputDevice = device.type == 'Relay' || device.type == 'Switch';
    
                if(mqttPanelElement !== null)
                    mqttPanelElement.style.display = event.target.checked ? 'block' : 'none';
    
                mqttFrequencyContainerElement.style.display = isOutputDevice ? 'none' : 'grid'
            });
    
        if(!mqttTopicElement !== null && device.mqtt.topic !== undefined) mqttTopicElement.value = device.mqtt.topic;
        if(!mqttFrequencyElement !== null && device.mqtt.frequency !== undefined)  mqttFrequencyElement.value = device.mqtt.frequency;
        
    
        
        if(toggleTriggerElement !== null)
            toggleTriggerElement.addEventListener('change', (event) => {
               
                if(triggerPanelElement !== null)
                    triggerPanelElement.style.display = event.target.checked ? 'block' : 'none';
            });
    
        if(deviceTriggerSourceElement !== null){
            //load up other devices into drop down from active config
            deviceTriggerSourceElement.innerHTML = '';
            if(device.trigger === undefined) device.trigger = {};
            for(var d of activeConfig.devices)
            {
                if(d.id == device.id) continue;
                const opt = document.createElement('option');
                opt.value = d.id;
                opt.textContent = d.name;
                deviceTriggerSourceElement.appendChild(opt);
            }
            deviceTriggerSourceElement.setAttribute('value',device.trigger.source);
            deviceTriggerSourceElement.value = device.trigger.source;
        }
    
        if(deviceTriggerTypeElement !== null){
            //deviceTriggerTypeElement.setAttribute('value',device.trigger.type);
            deviceTriggerTypeElement.value = device.trigger.type;
        }
    
        if(deviceTriggerValueElement !== null){
            //deviceTriggerValueElement.setAttribute('value',device.trigger.value);
            deviceTriggerValueElement.value = device.trigger.value;
        }
    
        if(deviceTriggerThresholdElement !== null){
            //deviceTriggerValueElement.setAttribute('value',device.trigger.value);
            deviceTriggerThresholdElement.value = device.trigger.threshold;
        }
    
        
        showModalComponent(deviceEditorElement,'Device Editor', 
            [
                { text:'Cancel',action: () => { closeModal();}},
                { text: 'Save', action:  () => { this.saveDevice();closeModal();}}
            ]
        );
    }
}




/* END OF DEVICES */