const deviceViewModelPath = '/W/M/device_model.json';
const deviceListViewModelPath = '/W/M/device_list_model.json'
/* DEVICES */
var activeConfig;
var persistedConfig;
//persistedDevices defined elsewhere
class esp32_devices{
    
    loadDevices(devices){
        persistedConfig = devices;
        activeConfig = devices;
    }
    openDeviceView(){
        //has data
        openView(
            'device-content',
            'Manage Devices',
            deviceListViewModelPath,
            activeConfig,
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
                                    activeConfig = activeConfig.filter(e => e.id != deviceItem.id);
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
                    showModal('ESP32 Settings Saved','Settings saved sucessfully. \nRestart device to apply settings? ', 
                    [
                        {text:'No',action: () => { closeModal();} }, 
                        {
                            text:'Yes', 
                            action: () => {
                                reset(true);closeModal(); 
                                setTimeout( () => window.location.reload(),5000)
                            }
                        }

                    ]);
                } else{
                    showModal('Unknown Error', 'An unknown error occured while saving. Please try again.');
                }
                                
            }
        }
        request.send(JSON.stringify(activeConfig));
    
    }
   
    
    //Save device edited in the form
    saveDevice(model){
        console.log('Saving device', model);

        if(model.id === undefined){
            model.id = activeConfig.length > 0 ? Math.max(...activeConfig.map(d => d.id)) + 1 : 1;            
        }
    
        //update in active config. refresh list
        if(activeConfig === undefined)
            activeConfig = [];

        const deviceIdx = activeConfig.findIndex(d => d.id == model.id);
        if(deviceIdx < 0)
            activeConfig.push(model);
        
        deviceManager.openDeviceView();
    
        
    
    }
    
    showDeviceEditor(device, onChangeCallback, onCancelCallback){
        openModal(
            'Device Editor',
            deviceViewModelPath,
            device,
            [
                {
                    Source: 'trigger.device', 
                    Field: 'Data', 
                    Value: activeConfig
                }
            ],
            this.saveDevice,
            onCancelCallback,
            null,
            onChangeCallback            
        );
    
        
    }
}
/* END OF DEVICES */



var deviceManager = new esp32_devices();