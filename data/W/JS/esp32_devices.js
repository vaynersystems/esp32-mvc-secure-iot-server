const deviceViewModelPath = '/W/M/device_model.json';
const deviceListViewModelPath = '/W/M/device_list_model.json';
const scheduleViewModelPath = '/W/M/schedule_model.json';
const scheduleEditModelPath = '/W/M/schedule_edit_model.json';
const deviceScheduleListViewModelPath = '/W/M/schedule_list_model.json';
/* DEVICES */
var activeConfig;
var persistedConfig;
var pins;
//persistedDevices defined elsewhere
class esp32_devices{
    
    loadDevices(devices, hwPins){
        persistedConfig = devices;
        activeConfig = devices;
        pins = hwPins;
    }
    openDeviceView(){
        //has data
        openView(
            'device-content',
            'Manage Devices',
            deviceListViewModelPath,
            activeConfig.devices,
            null,
            [
                {
                    'text':'Edit', action: (ev) =>{
                        var deviceItem = ev.currentTarget.record.Record;
                        this.showDeviceEditor(deviceItem,  (changed) => pendingChanges = changed);
                    }

                },
                {'text':'Delete', action: (ev) => {
                    var deviceItem = ev.currentTarget.record.Record;
                    showModal(
                        'Confirm Device Removal', 
                        'Are you sure you want to delete the <b>' + deviceItem.name + '</b> device?',
                        [
                            {text:'No', action: () => {closeModal()}}, 
                            {text: 'Yes', action: () => 
                                { 
                                    activeConfig.devices = activeConfig.devices.filter(e => e.id != deviceItem.id);
                                    pendingChanges = true;
                                    closeModal();
                                    this.openDeviceView();
                                } 
                            }
                        ]
                    )
                }}
            ],
            [{
                'text':'Add device', action: () => { 
                    var pendingBeforeAdd = pendingChanges; 
                    this.showDeviceEditor(
                        {},
                        (changed) => pendingChanges = changed,
                        () => pendingChanges = pendingBeforeAdd);
                }
            }]
           
        );
    }

    openScheduleView(){
        //has data
        openView(
            'device-content',
            'Manage Schedules',
            deviceScheduleListViewModelPath,
            activeConfig.schedules,
            [
                {
                    Source: 'devices', 
                    Field: 'Data', 
                    Value: activeConfig.devices.map(ac => Object.create({'name': ac.name, 'value': ac.id}))
                }
            ],
            [
                
                {
                    'text':'Edit', action: (ev) =>{
                        var scheduleItem = ev.currentTarget.record.Record;
                        this.showScheduleEditor(scheduleItem,  (changed) => pendingChanges = changed);
                    }

                },
                {'text':'Delete', action: (ev) => {
                    var scheduleItem = ev.currentTarget.record.Record;
                    showModal(
                        'Confirm Schedule Removal', 
                        'Are you sure you want to delete the <b>' + scheduleItem.name + '</b> schedule?',
                        [
                            {text:'No', action: () => {closeModal()}}, 
                            {text: 'Yes', action: () => 
                                { 
                                    activeConfig = activeConfig.schedules.filter(e => e.id != scheduleItem.id);
                                    pendingChanges = true;
                                    closeModal();
                                    this.openScheduleView();
                                } 
                            }
                        ]
                    )
                }}
            ],
            [{
                'text':'Add schedule entry', action: () => { 
                    var pendingBeforeAdd = pendingChanges; 
                    this.showScheduleEditor(
                        {},
                        (changed) => pendingChanges = changed,
                        () => pendingChanges = pendingBeforeAdd);
                }
            }]
           
        );
    }
    
    save(){  
        var waitElement = showWait();
        const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
        const url = base + "/" + 'SaveDeviceData';
        request.open("POST", url, true);
        request.setRequestHeader("Content-type", "application/json");
        request.onreadystatechange = function () {
            if (request.readyState == request.DONE) {
                if(waitElement !== undefined && waitElement !== null && waitElement.dispatchEvent !== undefined){
                    waitElement.dispatchEvent(new Event('close'));
            
                }
                if (request.status == 401) {
                    showModal('Unauthorized', '<p class="error">' + request.statusText + '</p>');                
                    return;
                }
                var response = request.responseText;
                if(request.status == 200){
                    pendingChanges = false;
                    if(request.responseURL.endsWith("ResetDevice")) {
                        closeModal();
                        return;
                    }
                    showModal('ESP32 Settings Saved','Settings saved sucessfully. \nRestart device to apply settings? ', 
                    [
                        {text:'No',action: () => { closeModal();} }, 
                        {
                            text:'Yes', 
                            action: () => {
                                setTimeout(() => {
                                    reset(true);closeModal(); 
                                    showWait();
                                    setTimeout( () => reload,10000);
                                }, 500);
                            }
                        }

                    ]);
                } else{
                    if(request.responseURL.includes("Save"))
                        showModal('Unknown Error', 'An unknown error occured requesting ' + request.responseURL + ' while saving. Please try again.');
                }
                                
            }
        }
        request.send(JSON.stringify(activeConfig));
    
    }
   
    
    //Save device edited in the form
    saveDevice(model){
        console.log('Saving device', model);

        if(model.id === undefined){
            model.id = activeConfig.devices.length > 0 ? Math.max(...activeConfig.devices.map(d => d.id)) + 1 : 1;            
        }
    
        //update in active config. refresh list
        if(activeConfig === undefined)
            activeConfig = [];

        const deviceIdx = activeConfig.devices.findIndex(d => d.id == model.id);
        if(deviceIdx < 0)
            activeConfig.devices.push(model);
        
        deviceManager.openDeviceView();
    
        
    
    }

    saveSchedule(model){
        console.log('Saving schedule', model);

        if(model.id === undefined){
            model.id = activeConfig.schedules.length > 0 ? Math.max(...activeConfig.schedules.map(d => d.id)) + 1 : 1;            
        }
    
        //update in active config. refresh list
        if(activeConfig === undefined)
            activeConfig = [];

        const deviceIdx = activeConfig.schedules.findIndex(d => d.id == model.id);
        if(deviceIdx < 0)
            activeConfig.schedules.push(model);
        
        deviceManager.openScheduleView();
    }
    saveScheduleConfiguration(model){
        alert('To be implemented!');
    }
    
    showDeviceEditor(device, onChangeCallback, onCancelCallback){
        openModal(
            'Device Editor',
            deviceViewModelPath,
            device,
            [
                {
                    Source: 'trigger.source', 
                    Field: 'Data', 
                    Value: activeConfig.devices.filter(ac => ac.id !== device.id).map(ac => Object.create({'name': ac.name, 'value': ac.id}))
                },
                {
                    Source: 'pin', 
                    Field: 'Data', 
                    Value: pins.filter(ac => ac.pin !== device.pin).map(ac => Object.create({'name': ac.pin, 'value': ac.pin}))
                }
            ],
            this.saveDevice,
            onCancelCallback,
            null,
            onChangeCallback            
        );
    
        
    }

    showScheduleEditor(schedule, onChangeCallback, onCancelCallback){
        openModal(
            'Schedule Editor',
            scheduleViewModelPath,
            schedule,
            [
                {
                    Source: 'devices', 
                    Field: 'Data', 
                    Value: activeConfig.devices.map(ac => Object.create({'name': ac.name, 'value': ac.id, 'group': ac.group}))
                },
                {
                    Source: 'trigger.source', 
                    Field: 'Data', 
                    Value: activeConfig.devices.map(ac => Object.create({'name': ac.name, 'value': ac.id, 'group': ac.group}))
                }
            ],
            this.saveSchedule,
            onCancelCallback,
            null,
            onChangeCallback            
        );
    
        
    }
}
/* END OF DEVICES */

function reload(){
    fetch('/')
    .then(() => window.location.reload())
    .catch( () => setTimeout(() => {
        reload
    }, 5000))
    ;
}


function confirmCancelChanges(cancelCallback){
    const modalComponent = _generateModalComponent(); 
    showModalComponent(
        modalComponent,
        'Unsaved Changes', 
        'You have unsaved changes. Are you sure you want to quit and lose these changes?',
        [{text:'No', action: () => {_destroyModalComponent(modalComponent);return false;}}, {text:'Yes', action: () => {_destroyModalComponent(modalComponent); closeModal(cancelCallback);}}]
    );
}

var deviceManager = new esp32_devices();
window.onbeforeunload = (event) => {
    
    if(pendingChanges){
        event.preventDefault();
        //return confirmCancelChanges((val) => {return val});
    }
}