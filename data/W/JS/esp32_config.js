var request = new XMLHttpRequest();
pendingChanges = false;
var persistedConfig = '';
var activeConfig;
var selectedSection;


function selectView(section){
    if(pendingChanges){
        const retval = confirm("Are you sure you want to quit without saving changes?")
        if(retval === false)
            return;
    }
    pendingChanges = false;

    if(selectedSection !== undefined)
    {
        var currentlyActiveSection = document.getElementById(selectedSection);
        if(currentlyActiveSection !== undefined && currentlyActiveSection !== null)
            currentlyActiveSection.className = '';
    }
    var selectionMenuElement = document.getElementById(section);
    if(selectionMenuElement !== undefined && selectionMenuElement !== null)
        selectionMenuElement.className = 'selected';

    var sectionElement = document.getElementsByClassName('section').namedItem(section);
    if(sectionElement === undefined) return;

    var contentElement = document.getElementById('config-content');
    if(contentElement === undefined) return;

    var sectionCollectionElement = document.getElementById('section-collection');
    if(sectionCollectionElement === undefined) return;

    //move any attached children back to body
    for (const child of contentElement.children) {
        sectionCollectionElement.appendChild(child);        
    }
    contentElement.appendChild(sectionElement);
    selectedSection = section;

    persistedConfig.preferences.lastPage = section;
}

/* WIFI  */
function selectWifiMode(wifiMode){        
    const modeSelectorElement = document.getElementById('network-mode');        
    const wifiAccessPointElement = document.getElementById('wifi-access-point');
    const wifiClientElement = document.getElementById('wifi-client');
    
    if(modeSelectorElement === undefined || wifiAccessPointElement === undefined || wifiClientElement === undefined) return;
    modeSelectorElement.value = wifiMode;

    const selectedComponentName = 'wifi-' + wifiMode;
    if(selectedComponentName == wifiAccessPointElement.id){
        wifiAccessPointElement.className = 'visible';
        wifiClientElement.className = 'hidden';
        const config = persistedConfig;       
        
        //load name, password, ip, subnet for access point
        document.getElementById('wifi-network-name').value = config.wifi.ap.name;
        document.getElementById('wifi-network-ap-password').value = config.wifi.ap.password;
        document.getElementById('wifi-network-ap-ip').value = config.wifi.ap.ip;
        document.getElementById('wifi-network-ap-subnet').value = config.wifi.ap.subnet;
    } else {
        wifiAccessPointElement.className = 'hidden';
        wifiClientElement.className = 'visible';      

        const networkNameElement = document.getElementById('wifi-network');
        const networkPasswordElement = document.getElementById('wifi-network-password');
        //var dropDownElement = networkNameElement === undefined ? ;
        if(networkNameElement === undefined) return;
        if(networkPasswordElement === undefined) return;
        const config = persistedConfig;
        var networkName = config.wifi.network;
        var networkPassword = config.wifi.password ?? ''; 
        if(networkName === undefined)
            getAvailableWifiNetworks();
        else
        {
            var option = document.createElement('option');               
            //var quality = strength < -70 ? "poor" : strength < -60 ? "decent" : strength < -40 ? "good" : "you're sitting on the transmitter";
            option.id = "wifi-" + networkName;
            option.textContent = networkName + "(persisted)";
            networkNameElement.appendChild(option);
        }
        const maskedPassword = networkPassword.length === 0 ? '' : '*'.repeat(networkPassword.length);
        networkPasswordElement.setAttribute('value', maskedPassword);
    }
    pendingChanges = true;
}

function selectDHCPMode(configuration){
    let isModeString = configuration.constructor === String;
    const dhcpOptions = document.getElementsByName('wifi-network-dhcp');
    const dhcpIPElement = document.getElementById('wifi-client-dhcp');
    const manualIPElement = document.getElementById('wifi-client-manual');
    var dhcpSelected = isModeString ? configuration == 'dhcp' : configuration.wifi.dhcp;
    for(let opt of dhcpOptions){
        if(opt.value == 'dhcp' && dhcpSelected){            
            opt.checked = true;
            dhcpIPElement.className = "";
            manualIPElement.className = "hidden";
            activeConfig.wifi.dhcp = true;
        }
        else  if(opt.value == 'manual' && !dhcpSelected){
            opt.checked = true;
            dhcpIPElement.className = "hidden";
            manualIPElement.className = "";      
            activeConfig.wifi.dhcp = false;                
        }            
    }    
    if(!dhcpSelected){
        loadIpSettings(activeConfig);          
    }
    pendingChanges = true;
}

function loadIpSettings(configuration){
    const ipElement = document.getElementById('wifi-network-client-ip');
    const subnetElement = document.getElementById('wifi-network-client-subnet');
    const gatewayElement = document.getElementById('wifi-network-client-gateway');
    const dnsElement = document.getElementById('wifi-network-client-dns');

    if(ipElement !== undefined && configuration.wifi.ip !== undefined){
        ipElement.value = configuration.wifi.ip;
    } 
    if(subnetElement !== undefined && configuration.wifi.subnet !== undefined){
        subnetElement.value = configuration.wifi.subnet;
    } 
    if(gatewayElement !== undefined && configuration.wifi.gateway !== undefined){
        gatewayElement.value = configuration.wifi.gateway;
    } 
    if(dnsElement !== undefined && configuration.wifi.dns !== undefined){
        dnsElement.value = configuration.wifi.dns;
    } 

}
function getAvailableWifiNetworks(){
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'GetAvailableWifi'
    request.open("GET", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            hideWait('wifi');
            if (request.status == 401) {
                document.getElementById('error').appendChild('<p class="error">' + request.responseText + '</p>');
                return;
            }
            var response = request.responseText;
            if(response.length <= 0 || response[0] !== '[') return;
            var networks = JSON.parse(response);
            //add networks to drop down
            var dropDownElement = document.getElementById('wifi-network');
            if(dropDownElement === undefined) return;

            dropDownElement.innerHTML = '';
            for(var network of networks){
                var option = document.createElement('option');
                var strength = network["rssi"];
                var quality = strength < -70 ? "poor" : strength < -60 ? "decent" : strength < -40 ? "good" : "you're sitting on the transmitter";
                option.id = "wifi-" + network["ssid"];
                option.textContent = network["ssid"] + "(" + quality + ")";
                dropDownElement.appendChild(option);
            }
            
        }
    }
    request.send(); 
    showWait('wifi');
}


/* END OF WIFI */

/* DEVICES */
deviceTypes = [
    {name: 'termometer', description: 'Thermometer', Type: 'input'},
    {name: 'relay', description: 'Relay', Type: 'output'}
]
function loadDevices(devices){
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
        editButtonElement.addEventListener('click', (event ) => { event.stopPropagation(); showDeviceEditor(device); });
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
        rowElement.addEventListener('click', () => showDeviceEditor(JSON.parse(rowElement.getAttribute('data'))))
        deviceListElement.appendChild(rowElement);
    }
}


function loadDeviceTypes(){
    const deviceList = document.getElementById('editor-device-type');
    if(deviceList === undefined || deviceList === null) return;

    for(var device in deviceTypes){
        const opt = document.createElement('opt');
        opt.value = device.name;
        opt.textContent = device.description;
    }

}

//Save device edited in the form
function saveDevice(){
    const modalElement = document.getElementById("system-modal-content");
    const triggerSelector = modalElement.querySelector('#editor-device-use-trigger');
    
    device = {};
    device.id = modalElement.querySelector('#editor-device-id').innerHTML;
    device.name = modalElement.querySelector('#editor-device-name').value;
    device.type = modalElement.querySelector('#editor-device-type').value;
    device.pin = modalElement.querySelector('#editor-device-pin').value;
    device.signal = modalElement.querySelector('#editor-device-signal').value;
    if(triggerSelector.checked){
    device.trigger = {};    
        device.trigger.active = triggerSelector.checked;
        device.trigger.source = modalElement.querySelector('#editor-device-trigger-source-device').value;
        device.trigger.type = modalElement.querySelector('#editor-device-trigger-type').value;
        device.trigger.value = modalElement.querySelector('#editor-device-trigger-value').value;
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
    
    loadDevices(activeConfig.devices);

    pendingChanges = true;

}
function showDeviceEditor(device){
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
            }
        }
    }

    const deviceIdElement = deviceEditorElement.querySelector('#editor-device-id');
    const deviceNameElement = deviceEditorElement.querySelector('#editor-device-name');
    const deviceTypeElement = deviceEditorElement.querySelector('#editor-device-type');
    const devicePinElement = deviceEditorElement.querySelector('#editor-device-pin'); 
    const deviceSignalElement = deviceEditorElement.querySelector('#editor-device-signal'); 
    const deviceTriggerSourceElement = deviceEditorElement.querySelector('#editor-device-trigger-source-device');
    const deviceTriggerTypeElement = deviceEditorElement.querySelector('#editor-device-trigger-type'); 
    const deviceTriggerValueElement = deviceEditorElement.querySelector('#editor-device-trigger-value'); 
    const triggerPanelElement = deviceEditorElement.querySelector('#editor-device-trigger-panel');
    const triggerContainerElement = deviceEditorElement.querySelector('#editor-device-use-trigger-container');
    const toggleTriggerElement = deviceEditorElement.querySelector('#editor-device-use-trigger');
    const deviceSignalContainerElement = deviceEditorElement.querySelector('#editor-device-signal-container');
    
    if(deviceIdElement !== null) deviceIdElement.innerHTML =  device.id;
    if(deviceNameElement !== null) deviceNameElement.value = device.name;
    if(deviceTypeElement !== null) {
        if(device.type !== undefined) deviceTypeElement.value = device.type;
        toggleTriggerElement.checked =  device.trigger === undefined ? false : device.trigger.active;
        // if(device.type.length > 0) deviceTypeElement.value = device.type;
        isOutputDevice = device.type == 'relay' || device.type == 'switch';
        triggerContainerElement.style.display = isOutputDevice ? 'block' : 'none'; 
        deviceSignalContainerElement.style.display = isOutputDevice ? '' : 'none'; 
        
        triggerPanelElement.style.display = isOutputDevice && toggleTriggerElement.checked ? 'block' : 'none'; 

        deviceTypeElement.addEventListener('change',(event) => {
            isOutputDevice = event.target.value == 'relay' || event.target.value == 'switch';
            triggerContainerElement.style.display = isOutputDevice ? '' : 'none'; 
            deviceSignalContainerElement.style.display = isOutputDevice ? '' : 'none'; 
            triggerPanelElement.style.display = isOutputDevice && toggleTriggerElement.checked ? 'block' : 'none'; 
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
        if(device.signal.length > 0) deviceSignalElement.value = device.signal;
    }

    
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

    
    showModalComponent(deviceEditorElement,'Device Editor', 
        [
            { text:'Cancel',action: () => { closeModal();}},
            { text: 'Save', action:  () => { saveDevice();closeModal();}}
        ]
    );
}



/* END OF DEVICES */
var publicCertFile = undefined;
var privateCertFile = undefined;
var publicSaved = 0, privateSaved = 0, certificateGenerated=0;
function showCertificateAction(){
    

    const selectedAction = document.getElementById('certificate-action').value;
    if(selectedAction == 'upload'){
        //clone form to use in modal
        publicSaved = 0;
        privateSaved = 0;
        const certificateUploadSourceElement = document.getElementById('editor-certificate-upload');
        if(certificateUploadSourceElement === null) return;
        const certificateUploadElement = certificateUploadSourceElement.cloneNode(true);
        certificateUploadElement.setAttribute('modal', certificateUploadElement.id);

        //wire up change event for file dialogs
        const publicCertFileElement = certificateUploadElement.querySelector('#public-cert');
        const privateCertFileElement = certificateUploadElement.querySelector('#private-cert');
        if(publicCertFileElement === null || privateCertFileElement === null) return;
        publicCertFileElement.addEventListener('change', () =>{
            if(publicCertFileElement.files.length > 0)
                publicCertFile = publicCertFileElement.files[0];
        })

        privateCertFileElement.addEventListener('change', () =>{
            if(privateCertFileElement.files.length > 0)
                privateCertFile = privateCertFileElement.files[0];
        })
        
        showModalComponent(certificateUploadElement,'Upload a certificate', 
        [
            { text:'Cancel',action: () => { publicCertFile = undefined; privateCertFile = undefined; closeModal();}},
            { text: 'Upload', action:  () => { uploadCertificates(); }}
        ], '800px');
        

    }else if(selectedAction == 'generate'){
        certificateGenerated = 0;
        const certificateGenerateSourceElement = document.getElementById('editor-certificate-generate');
        if(certificateGenerateSourceElement === null) return;
        const certificateGenerateElement = certificateGenerateSourceElement.cloneNode(true);
        certificateGenerateElement.setAttribute('modal', certificateGenerateElement.id);

        if(publicCertFile === undefined)
        
        showModalComponent(certificateGenerateElement,'Generate a new certificate', 
        [
            { text:'Cancel',action: () => { closeModal();}},
            { text: 'Generate', action:  () => { generateCertificates();  }}
        ]);
    }
}
var startTime = undefined;
function uploadCertificates(){
    showWait();
    //call backend to save files in temp path.
    //set activeConfig's certificate object to target those paths
    //on form save, if paths exist, backend will move certificates to the configured storage
    if(publicCertFile === undefined || privateCertFile === undefined) return;
   

    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;

    //send public
    var publicRequest = new XMLHttpRequest();
    const urlPublic = base + "/" + 'UploadCertificate?Public';    
    publicRequest.open("POST", urlPublic, true);
    publicRequest.onreadystatechange = function () {
        if (publicRequest.readyState == request.DONE) {
            
            if(publicRequest.status == 200){
                var response = publicRequest.responseText;
                publicSaved = 1;
                return;
            }
            //hideWait('page');
            if (publicRequest.status == 401) {
                showModal('<p class="error">' + publicRequest.statusText + '</p>', 'Unauthorized');                   
            }
            publicSaved = -1;                            
        }
    }
    const publicFileData = new FormData();    
    publicFileData.append("file", publicCertFile, "public.cer");
    publicRequest.send(publicFileData);

    //send private
    var privateRequest = new XMLHttpRequest();
    const urlPrivate = base + "/" + 'UploadCertificate?Private';   
    privateRequest.open("POST", urlPrivate, true);
    privateRequest.onreadystatechange = function () {
        if (privateRequest.readyState == request.DONE) {
            if(privateRequest.status == 200){
                privateSaved = 1;
                return;
            }
            //hideWait('page');
            if (privateRequest.status == 401) {
                showModal('<p class="error"> UNAUTHORIZED! ' + privateRequest.statusText + '</p>', 'Unauthorized');                                              
            }
            privateSaved = -1; 
        }
    }
    const privateFileData = new FormData();
    privateFileData.append("file", privateCertFile, "private.key");
    privateRequest.send(privateFileData);
    startTime = new Date();
    checkIfUploaded();
}

//will run for up to one minute
function checkIfUploaded(){
    const secondsSinceStarted = (new Date().getTime() - startTime.getTime())/1000;
    if((publicSaved == 0 || privateSaved == 0) && secondsSinceStarted < 30)
    {
        setTimeout(checkIfUploaded, 1000);
        return;
    }
    hideWait();
    //if uploaded, user is done
    if(publicSaved > 0 && privateSaved > 0){
        closeModal();
        pendingChanges = true;
        activeConfig.server.certificates.uploaded = true;
    }
        
}

function generateCertificates(){
    //set activeConfig certificate object to generate info specified
    //on form save, backend will generate certificate to the configured storage
    const certForm = document.querySelector('[modal="editor-certificate-generate"]');
    if(certForm === undefined){
        alert('Error in system. Form cannot be found. Reload page and try again.');
        return;
    }

    const certDeviceNameElement = certForm.querySelector('#device-name');
    const certCompanyNameElement = certForm.querySelector('#company-name');
    const certValidFromNameElement = certForm.querySelector('#valid-from');
    const certValidToNameElement = certForm.querySelector('#valid-to');
    if(certDeviceNameElement == null || certCompanyNameElement === null || certValidFromNameElement === null || certValidToNameElement === null)
        return;

    showWait();

    generateRequestData = {};
    generateRequestData.device = certDeviceNameElement.value;
    generateRequestData.company = certCompanyNameElement.value;
    generateRequestData.from = certValidFromNameElement.value.split('-').join('') + '000000';
    generateRequestData.to = certValidToNameElement.value.split('-').join('') + '000000';  
    
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;

    //send public
    var generateRequest = new XMLHttpRequest();
    const urlPublic = base + "/" + 'GenerateCertificate';    
    generateRequest.open("POST", urlPublic, true);
    generateRequest.onreadystatechange = function () {
        if (generateRequest.readyState == request.DONE) {
            hideWait();
            if(generateRequest.status == 200){
                var response = generateRequest.responseText;
                certificateGenerated = 1;
                activeConfig.server.certificates.uploaded = true;
                closeModal();
                return;
            }
            //hideWait('page');
            if (generateRequest.status == 401) {
                showModal('<p class="error">' + generateRequest.statusText + '</p>', 'Unauthorized');                   
            }            
        }
    }
    
    generateRequest.send(JSON.stringify(generateRequestData));

    pendingChanges = true;
}


function loadSettings(){
    
    //overall
    var view = activeConfig.preferences === undefined ? 'devices' : activeConfig.preferences.lastPage;
    selectView(view);

    //wifi
    selectWifiMode(activeConfig.wifi.mode);
    selectDHCPMode(activeConfig);
    //system
    const hostNameElement = document.getElementById('host-name');
    const hostEnableSSLElement = document.getElementById('host-enable-ssl');
    const hostEnableMDNSElement = document.getElementById('host-enable-mdns');
    
    
    const ntpServerElement = document.getElementById('ntp-host-name');
    const timeZoneElement = document.getElementById('time-zone');

    if(hostNameElement !== null) hostNameElement.value = activeConfig.system.hostname;
    if(ntpServerElement !== null) ntpServerElement.value = activeConfig.system.ntp.server;
    if(timeZoneElement !== null) timeZoneElement.value = activeConfig.system.ntp.timezone;

    if(hostEnableSSLElement !== null) hostEnableSSLElement.checked = activeConfig.system.enableSSL;
    if(hostEnableMDNSElement !== null) hostEnableMDNSElement.value = activeConfig.system.enableMDNS;

    //server
    const disableWifiElement = document.getElementById('disable-wifi-timer');
    if(disableWifiElement !== null) disableWifiElement.value = activeConfig.server.disableWifiTimer;

    const certificateSourceElements = document.getElementsByName('certificate-source');
    for(var child of certificateSourceElements){
        child.checked = (child.value === activeConfig.server.certificates.source);
    }

    //devices
    loadDevices(activeConfig.devices);
    pendingChanges = false;
    
}
function seveSettingsFromForm(){
    var config = persistedConfig;
    //devices
    config.devices = activeConfig.devices;
    if(config.devices === undefined)
        config.devices = [];


    //schedules
    if(config.schedules === undefined)
        config.schedules = [];

    //system
    if(config.system === undefined)
        config.system = {};    
    config.system.hostname = document.getElementById('host-name').value ?? config.system.hostname;
    config.system.ntp.server = document.getElementById('ntp-host-name').value;
    config.system.ntp.timezone = document.getElementById('time-zone').value;
    const hostEnableSSLElement = document.getElementById('host-enable-ssl');
    const hostEnableMDNSElement = document.getElementById('host-enable-mdns');
    if(hostEnableSSLElement !== null) config.system.enableSSL = document.getElementById('host-enable-ssl').checked;
    if(hostEnableMDNSElement !== null) config.system.enableMDNS = document.getElementById('host-enable-mdns').checked;

    const certificateSourceElement = document.querySelector('input[name="certificate-source"]:checked');

    //server
    if(config.server === undefined)
        config.server = {};
    config.server.disableWifiTimer = document.getElementById('disable-wifi-timer').value;

    if(config.server.certificates === undefined)
        config.server.certificates = {};
    
    config.server.certificates.source = certificateSourceElement.value;
    //upload and generate actions already set active config changes
    

    //wifi
    if(config.wifi === undefined)
        config.wifi = {};
    config.wifi.mode = document.getElementById('network-mode')?.value ?? 'client';  
    if(config.wifi.mode == 'client'){     
        const fullNetworkString = document.getElementById('wifi-network').value;
        const docPassword = document.getElementById('wifi-network-password').value;
        const hasExtraInfo = fullNetworkString.indexOf('(') > 0;
        const networkName = hasExtraInfo ? fullNetworkString.substring(0,fullNetworkString.indexOf('(')) : fullNetworkString;
        config.wifi.network = networkName;
        config.wifi.password = docPassword.replaceAll('*','').length > 0 ?  docPassword : config.wifi.password;
        config.wifi.dhcp = document.getElementById('wifi-network-dhcp-yes').checked;
        if(!config.wifi.dhcp){
            if(!checkOctects('wifi-network-client-ip')) return;
            if(!checkOctects('wifi-network-client-subnet')) return;
            if(!checkOctects('wifi-network-client-gateway')) return;
            if(!checkOctects('wifi-network-client-dns')) return;
            config.wifi.ip = document.getElementById('wifi-network-client-ip')?.value;
            config.wifi.subnet = document.getElementById('wifi-network-client-subnet')?.value;
            config.wifi.gateway = document.getElementById('wifi-network-client-gateway')?.value;
            config.wifi.dns = document.getElementById('wifi-network-client-dns')?.value;
            
        }
    }
    if(config.wifi.mode == 'access-point'){
        if(config.wifi.ap === undefined)
            config.wifi.ap = {};
        config.wifi.ap.name = document.getElementById('wifi-network-name')?.value;
        config.wifi.ap.password = document.getElementById('wifi-network-ap-password')?.value;
        config.wifi.ap.ip = document.getElementById('wifi-network-ap-ip')?.value;
        config.wifi.ap.subnet = document.getElementById('wifi-network-ap-subnet')?.value;
    }
    config.type = 'esp32-config'
    saveSettings(config);
}
//parse form, collecting all of the input, provide as json to post
function saveSettings(config){
    //TODO: bind controls to activeConfiguration model, just persist it here   

    console.log('calling backend to save settings ', config);
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'SaveConfigData';
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {

            hideWait('page');
            if (request.status == 401) {
                showModal('<p class="error">' + request.statusText + '</p>', 'Unauthorized');                
                return;
            }
            var response = request.responseText;
            if(request.status == 200){
                showModal('Settings saved sucessfully. \nRestart device to apply settings? ','ESP32 Settings Saved', [{text:'No',action: () => { closeModal();} }, {text:'Yes', action: () => {reset(true);closeModal();}}]);
            }
                            
        }
    }
    request.send(JSON.stringify(config));
    showWait('page');
}



function restoreSettings() {
    var input = document.getElementById("restoreSettings");
    var reader = new FileReader();
    if ("files" in input) {
        if (input.files.length === 0) {
            alert("You did not select any file to restore");
        } else {
            reader.onload = function () {
                var json;
                try {
                    json = JSON.parse(reader.result);
                } catch (e) {
                    alert("Not a valid backup file");
                    return;
                }
                if (json.type === "esp32-config") {
                    var writeToDevice = confirm("You are about to overwrite your device settings. Are you sure you want to continue?");
                    if(writeToDevice)
                        saveSettings(json);
                }
            };
            reader.readAsText(input.files[0]);
        }
    }
}


function esp32_config_init(configDataSting){
    
    document.getElementById('network-mode').addEventListener('change', function(){
        const modeSelectorElement = document.getElementById('network-mode');
    
        if(modeSelectorElement === undefined) return;
        selectWifiMode(modeSelectorElement.value);
    })

    const links = document.getElementsByTagName('li');
    for(let link of links){
        link.addEventListener('click', (e) => {selectView(link.id)});
    }

    const certificateAction = document.getElementById('certificate-action-button');    
    certificateAction.addEventListener('click', () => showCertificateAction())
    
    const dhcpOptions = document.getElementsByName('wifi-network-dhcp');
    for(let opt of dhcpOptions){
        opt.addEventListener('change', (e) => { selectDHCPMode(e.target.value); })
    }
    const themeSelectorElement = document.getElementById('system-theme');
    if(themeSelectorElement !== null){
        themeSelectorElement.addEventListener('change', (event) => setTheme(event.target.value));
    }
    if(configDataSting === "$_CONFIGURATION_DATA")
        persistedConfig = {};
    else 
        persistedConfig = JSON.parse(configDataSting);

    if(persistedConfig.type === undefined){
        persistedConfig.type = 'esp32-config';
    }

    if(persistedConfig.devices === undefined){
        persistedConfig.devices = [];
    }

    if(persistedConfig.wifi === undefined || persistedConfig.wifi.mode === undefined){
        persistedConfig.wifi = {};
        persistedConfig.wifi.mode = 'client';
        persistedConfig.wifi.dhcp = true;
        persistedConfig.preferences = {};

        persistedConfig.preferences.lastPage = 'devices';   
    }
    if(persistedConfig.system === undefined){
        persistedConfig.system = {};
        persistedConfig.system.hostname = 'ESP32-Dev-Host' + new Date().getMinutes(); 
        persistedConfig.system.enableSSL = true;
        persistedConfig.system.enableMDNS = false;
    }
    if(persistedConfig.system.ntp === undefined){
        persistedConfig.system.ntp = {};
        persistedConfig.system.ntp.server = 'us.pool.ntp.org';
        persistedConfig.system.ntp.timezone = '-5';
    }

    if(persistedConfig.server === undefined)
        persistedConfig.server = {};

    if(persistedConfig.server.disableWifiTimer === undefined){
        persistedConfig.server.disableWifiTimer = 'never'
    }

    if(persistedConfig.server.certificates === undefined){
        persistedConfig.server.certificates = {};
        persistedConfig.server.certificates.source = 'nvs';        
    }    

    activeConfig = persistedConfig;

    loadSettings();
    hideLoading();

    //start checking for pending changes
    checkPendingChanges();
    
}

///

function setTheme(theme){

    if(theme == 'light'){
        document.body.style.setProperty('--primary-color','#191955');
        document.body.style.setProperty('--primary-border', 'rgb(164, 190, 216)');
        document.body.style.setProperty('--primary-background','#f4f5f1');
        document.body.style.setProperty('--secondary-color','#d0e9d1');
        document.body.style.setProperty('--secondary-background','#191955');
        document.body.style.setProperty('--primary-control-color','#053f16');
        document.body.style.setProperty('--primary-control-background','#ffffff');
        document.body.style.setProperty('--primary-button-color','#edeeef');
        document.body.style.setProperty('--primary-button-background','#0b2d64');
        document.body.style.setProperty('--secondary-button-color','#edeeef');
        document.body.style.setProperty('--secondary-button-background','#1d8e56');
        document.body.style.setProperty('--secondary-button-background-hover','#11633a');
        document.body.style.setProperty('--modal-color','#234233');
        document.body.style.setProperty('--modal-background', '#31863147');
        document.body.style.setProperty('--modal-header-color','#fff');
        document.body.style.setProperty('--modal-header-background','#1d8e56');
    }
    else{
        document.body.style.setProperty('--secondary-color','#191955');
        document.body.style.setProperty('--secondary-border', 'rgb(164, 190, 216)');
        document.body.style.setProperty('--secondary-background','#f4f5f1');
        document.body.style.setProperty('--primary-color','#f6f6ff');
        document.body.style.setProperty('--primary-background','#050520');
        document.body.style.setProperty('--primary-control-color','#ffffff');
        document.body.style.setProperty('--primary-control-background','#0c0c4e');
        document.body.style.setProperty('--secondary-button-color','#0b2d64');
        document.body.style.setProperty('--secondary-button-background','#edeeef');
        document.body.style.setProperty('--primary-button-color','#edeeef');
        document.body.style.setProperty('--primary-button-background','#1d8e56');
        document.body.style.setProperty('--primary-button-background-hover','#11633a');
        document.body.style.setProperty('--modal-color','#fff');
        document.body.style.setProperty('--modal-background', 'rgba(3, 45, 9, 0.4)');
        document.body.style.setProperty('--modal-header-color','#fff');
        document.body.style.setProperty('--modal-header-background','#1d8e56');

        

    }
}

function checkPendingChanges(){
    const pendingElement = document.getElementById('pending-changes');
    if(pendingElement === null) return;
    pendingElement.style.visibility = pendingChanges ? 'visible' : 'hidden';
    setTimeout(checkPendingChanges, 500);
}

//should be moved to a global file



function checkOctects(input) {
    var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
    var call = document.getElementById(input);
    if (call.value.match(ipformat)) {
        return true;
    } else {
        alert("You have entered an invalid address on " + input);
        call.focus();
        return false;
    }

}


//ran when first parsed
document.addEventListener("DOMContentLoaded", function() {
    showLoading();
});
