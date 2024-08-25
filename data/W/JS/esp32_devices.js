const deviceViewModelPath = '/W/M/device_model.json';

/* DEVICES */
class esp32_devices{

    loadDevices(devices){
        const deviceListElement = document.getElementById('device-list');
        if(deviceListElement === null) return;
    
        deviceListElement.innerHTML = '';
    
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
                    'Confirm Device Removal', 
                    'Are you sure you want to delete the <b>' + rowElement.getAttribute('device-name') + '</b> device?',
                    [
                        {text:'No', action: () => {closeModal()}}, 
                        {text: 'Yes', action: () => 
                            { 
                                activeConfig.devices = activeConfig.devices.filter(e => e.id != device.id);
                                closeModal();
                                this.loadDevices(activeConfig.devices);
                                pendingChanges = true;
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
    
   
    
    //Save device edited in the form
    saveDevice(model){
        console.log('Saving device', model);

        if(model.id === undefined){
            model.id = activeConfig.devices.length > 0 ? Math.max(...activeConfig.devices.map(d => d.id)) + 1 : 1;            
        }
    
        //update in active config. refresh list
        if(activeConfig.devices === undefined)
            activeConfig.devices = [];
    
        const deviceIdx = activeConfig.devices.findIndex(d => d.id == model.id);
        if(deviceIdx >= 0) // replace
            activeConfig.devices.splice(deviceIdx,1,model);
        else
            activeConfig.devices.push(model);
        
            devices.loadDevices(activeConfig.devices);
    
        pendingChanges = true;
    
    }
    
    showDeviceEditor(device){
        openModal(
            'Device Editor',
            deviceViewModelPath,
            device,
            [
                {
                    Source: 'trigger.device', 
                    Field: 'Data', 
                    Value: activeConfig.devices
                }
            ],
            this.saveDevice            
        );
    
        
    }
}
/* END OF DEVICES */