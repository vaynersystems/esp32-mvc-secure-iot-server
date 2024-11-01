var request = new XMLHttpRequest();
pendingChanges = false;
var persistedConfig = '';
var activeConfig;
var selectedSection;

function selectView(section){
    
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


function formatByte(value){
    if(value/1024/1024 > 1.05)
        return Math.round((value/1024/1024) * 100) / 100 + ' MB';
    if(value/1024 > 1.05)
        return Math.round((value/1024) * 100) / 100 + ' KB';
    return value + ' B';
}
function getProjectionColor(bytes){
    const loggingStorageElement = document.getElementById('logging-location');
    var diskSize = drives[loggingStorageElement.value].size;

    if(bytes > (diskSize * .8)) return 'red';
    if(bytes > (diskSize * .4)) return 'orange';
    return 'green';
}

/* Logging  */
function showLoggingProjections(){
    const loggingFrequencyElement = document.getElementById('device-logging-frequency');
    const frequency = loggingFrequencyElement === null ? 300 : loggingFrequencyElement.value;

    const envelope = 35;
    const messageLength = 32;
    
    const projectionContainerElement = document.getElementById('device-logging-projection');

    
    
    if(projectionContainerElement === null) return;
    projectionContainerElement.innerHTML = '';
    for(var deviceCount = 1; deviceCount < 8; deviceCount++){
        const entriesPerDay = ((24*3600)/frequency) * deviceCount;
        const bytesPerDay = entriesPerDay * messageLength + envelope;
        const projectionRowElement = document.createElement('div');
        const deviceCell = document.createElement('div');
        const dailyCell = document.createElement('div');
        const weeklyCell = document.createElement('div');
        const monthlyCell = document.createElement('div');
        const sixMonthsCell = document.createElement('div');
        const yearlyCell = document.createElement('div');
        deviceCell.textContent = deviceCount;        
        dailyCell.textContent = formatByte(bytesPerDay);
        dailyCell.style.color = getProjectionColor(bytesPerDay);
        weeklyCell.textContent = formatByte(bytesPerDay * 7);
        weeklyCell.style.color = getProjectionColor(bytesPerDay * 7);
        monthlyCell.textContent = formatByte(bytesPerDay * 30);
        monthlyCell.style.color = getProjectionColor(bytesPerDay * 30);
        sixMonthsCell.textContent = formatByte(bytesPerDay * 182);
        sixMonthsCell.style.color = getProjectionColor(bytesPerDay * 182);        
        yearlyCell.textContent = formatByte(bytesPerDay * 365);
        yearlyCell.style.color = getProjectionColor(bytesPerDay * 365);
        projectionRowElement.className = "grid-row grid-6";

        projectionRowElement.appendChild(deviceCell); 
        projectionRowElement.appendChild(dailyCell);
        projectionRowElement.appendChild(weeklyCell);
        projectionRowElement.appendChild(monthlyCell);
        projectionRowElement.appendChild(sixMonthsCell);
        projectionRowElement.appendChild(yearlyCell);
        projectionContainerElement.appendChild(projectionRowElement);
    }


}


function loadSettings(){
    
    //overall
    var view = activeConfig.preferences === undefined ? 'general' : activeConfig.preferences.lastPage;
    selectView(view);

    //wifi
    selectWifiMode(activeConfig.wifi.mode);
    selectDHCPMode(activeConfig);
    //system
    const hostNameElement = document.getElementById('host-name');
    const hostEnableSSLElement = document.getElementById('host-enable-ssl');
    const hostEnableMDNSElement = document.getElementById('host-enable-mdns');
    const enableEditorElement = document.getElementById('enable-editor');
    const enableFingerprintsElement = document.getElementById('enable-fingerprints');
    const mqttConstraintsElement = document.getElementById('mqtt-constraints-warning');
    const mqttPublishSectionElement = document.getElementById('mqtt-publish-section');
    
    
    const ntpServerElement = document.getElementById('ntp-host-name');
    const timeZoneElement = document.getElementById('time-zone');

    const loggingFrequencyElement = document.getElementById('device-logging-frequency');
    const loggingRetentionElement = document.getElementById('logging-retention');
    const loggingLevelElement = document.getElementById('logging-level');
    const loggingLocationElement = document.getElementById('logging-location');

    const mqttEnabledElement = document.getElementById('mqtt-enabled');
    const mqttHostElement = document.getElementById('mqtt-broker');
    const mqttPortElement = document.getElementById('mqtt-port');
    const mqttInsecureElement = document.getElementById('mqtt-insecure');
    const mqttCertSkipVerificationContainerElement = document.getElementById('skip-cert-verification-container');
    

    if(hostNameElement !== null) hostNameElement.value = activeConfig.system.hostname;
    if(ntpServerElement !== null) ntpServerElement.value = activeConfig.system.ntp.server;
    if(timeZoneElement !== null) timeZoneElement.value = activeConfig.system.ntp.timezone;

    if(hostEnableSSLElement !== null) hostEnableSSLElement.checked = activeConfig.system.enableSSL;
    if(hostEnableMDNSElement !== null) hostEnableMDNSElement.checked = activeConfig.system.enableMDNS;

    if(enableEditorElement !== null) enableEditorElement.checked = activeConfig.system.features.enableEditor;
    if(enableFingerprintsElement !== null) enableFingerprintsElement.checked = activeConfig.system.features.enableFingerprints;

    

    if(loggingFrequencyElement !== null) loggingFrequencyElement.value = activeConfig.system.logging.frequency;
    if(loggingRetentionElement !== null) loggingRetentionElement.value = activeConfig.system.logging.retention;
    if(loggingLevelElement !== null) loggingLevelElement.value = activeConfig.system.logging.level;
    if(loggingLocationElement !== null) loggingLocationElement.value = activeConfig.system.logging.location;
    


    if(mqttEnabledElement !== null) mqttEnabledElement.value = activeConfig.system.mqtt.enabled;
    if(mqttHostElement !== null) mqttHostElement.value = activeConfig.system.mqtt.broker;
    if(mqttPortElement !== null) mqttPortElement.value = activeConfig.system.mqtt.port;
    if(mqttInsecureElement !== null) mqttInsecureElement.value = activeConfig.system.mqtt.insecure;

    if(mqttEnabledElement !== null){
        mqttEnabledElement.checked = activeConfig.system.mqtt.enabled;
        if(activeConfig.system.mqtt.enabled){
            
            mqttHostElement.removeAttribute('disabled');
            mqttPortElement.removeAttribute('disabled');
            mqttInsecureElement.removeAttribute('disabled');
            mqttPublishSectionElement.style.display = 'flex';
            mqttConstraintsElement.style.display = 'grid';
        } else{
            mqttHostElement.setAttribute('disabled','true');
            mqttPortElement.setAttribute('disabled','true');
            mqttInsecureElement.setAttribute('disabled','true');
            mqttPublishSectionElement.style.display = 'none';            
            mqttConstraintsElement.style.display = 'none';
        }
        mqttEnabledElement.addEventListener('change', (ev) => {
            if(ev.target.checked){
                mqttHostElement.removeAttribute('disabled');
                mqttPortElement.removeAttribute('disabled');
                mqttInsecureElement.removeAttribute('disabled');
                mqttConstraintsElement.style.display = 'grid';
                mqttPublishSectionElement.style.display = 'flex';
                
            } else{
                mqttHostElement.setAttribute('disabled','true');
                mqttPortElement.setAttribute('disabled','true');
                mqttInsecureElement.setAttribute('disabled','true');
                mqttPublishSectionElement.style.display = 'none';
                mqttConstraintsElement.style.display = 'none';
            }
        })
    }

    if(mqttPortElement !== null && mqttCertSkipVerificationContainerElement != null){
        mqttPortElement.addEventListener('change', (ev) => {
            mqttCertSkipVerificationContainerElement.style.display = ev.target.value == 1883 ? 'none' : 'grid';
            //mqttConstraintsElement.style.display = ev.target.value == 1883 ? 'none' : 'grid';
        })
        mqttCertSkipVerificationContainerElement.style.display = mqttPortElement.value == 1883 ? 'none' : 'grid';
        //mqttConstraintsElement.style.display = mqttPortElement.value == 1883 ? 'none' : 'grid';
    }


    //server
    const disableWifiElement = document.getElementById('disable-wifi-timer');
    if(disableWifiElement !== null) disableWifiElement.value = activeConfig.server.disableWifiTimer;

    const restartAfterElement = document.getElementById('restart-after');
    if(restartAfterElement !== null) restartAfterElement.value = activeConfig.server.restartAfter;

    const certificateSourceElements = document.getElementsByName('certificate-source');
    for(var child of certificateSourceElements){
        child.checked = (child.value === activeConfig.server.certificates.source);
    }

    pendingChanges = false;
    
}
function seveSettingsFromForm(){
    showLoading();
    var config = persistedConfig;
   
    //schedules
    // if(config.schedules === undefined)
    //     config.schedules = [];

    //system
    if(config.system === undefined)
        config.system = {};    
    config.system.hostname = document.getElementById('host-name').value ?? config.system.hostname;
    config.system.ntp.server = document.getElementById('ntp-host-name').value;
    config.system.ntp.timezone = document.getElementById('time-zone').value;
    const hostEnableSSLElement = document.getElementById('host-enable-ssl');
    const hostEnableMDNSElement = document.getElementById('host-enable-mdns');

    const enableEditorElement = document.getElementById('enable-editor');
    const enableFingerprintsElement = document.getElementById('enable-fingerprints');
   
    const loggingFrequencyElement = document.getElementById('device-logging-frequency');
    const loggingRetentionElement = document.getElementById('logging-retention');
    const loggingLevelElement = document.getElementById('logging-level');
    const loggingLocationElement = document.getElementById('logging-location');
    const mqttHostElement = document.getElementById('mqtt-broker');
    const mqttPortElement = document.getElementById('mqtt-port');
    const mqttInsecureElement = document.getElementById('mqtt-insecure');   
    const mqttEnabledElement = document.getElementById('mqtt-enabled'); 
    const mqttSubscribeEnabledElement = document.getElementById('mqtt-subscribe-enabled');
    
    
    if(hostEnableSSLElement !== null) config.system.enableSSL = hostEnableSSLElement.checked;
    if(hostEnableMDNSElement !== null) config.system.enableMDNS = hostEnableMDNSElement.checked;

    if(enableEditorElement !== null) config.system.features.enableEditor = enableEditorElement.checked;
    if(enableFingerprintsElement !== null) config.system.features.enableFingerprints = enableFingerprintsElement.checked;

    if(loggingFrequencyElement !== null) activeConfig.system.logging.frequency = loggingFrequencyElement.value;
    if(loggingRetentionElement !== null) activeConfig.system.logging.retention = loggingRetentionElement.value;
    if(loggingLevelElement !== null) activeConfig.system.logging.level = loggingLevelElement.value;
    if(loggingLocationElement !== null) activeConfig.system.logging.location = loggingLocationElement.value;

    if(mqttHostElement !== null) activeConfig.system.mqtt.broker = mqttHostElement.value;
    if(mqttPortElement !== null) activeConfig.system.mqtt.port = mqttPortElement.value;
    if(mqttInsecureElement !== null) activeConfig.system.mqtt.insecure = mqttInsecureElement.checked;
    if(mqttEnabledElement !== null) activeConfig.system.mqtt.enabled = mqttEnabledElement.checked;
    if(mqttSubscribeEnabledElement !== null) activeConfig.system.mqtt.subscribeEnabled = mqttSubscribeEnabledElement.checked;
    
    const certificateSourceElement = document.querySelector('input[name="certificate-source"]:checked');

    //server
    if(config.server === undefined)
        config.server = {};
    config.server.disableWifiTimer = document.getElementById('disable-wifi-timer').value;
    config.server.restartAfter = document.getElementById('restart-after').value;

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
        if(config.wifi.ap.password.length < 8) {
            alert('Acess Point password should be at leat 8 characters long');
        }
    }
    config.type = 'esp32-config'
    saveSettings(config);
}
//parse form, collecting all of the input, provide as json to post
function saveSettings(config){
    //TODO: bind controls to activeConfiguration model, just persist it here   
    //showWait('page');
    console.log('calling backend to save settings ', config);
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'SaveConfigData';
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            hideLoading();
            //hideWait('page');
            if (request.status == 401) {
                showModal('Unauthorized', '<p class="error">' + request.statusText + '</p>');                
                return;
            }
            var response = request.responseText;
            if(request.status == 200){
                pendingChanges = false;
                showModal('ESP32 Settings Saved', 'Settings saved sucessfully. \nRestart device to apply settings? '
                [
                    {text:'No',action: () => { closeModal();} }, 
                    {
                        text:'Yes', 
                        action: () => {
                            setTimeout(() => 
                            {
                                reset(true);closeModal(); 
                                showWait();
                                setTimeout( reload,5000);
                            })   
                        }                     
                    }

                ]);
            } else{
                if(request.status == 0 && request.responseURL.endsWith("ResetDevice")){
                    console.log('Updates applied and device reset!')
                }else
                    showModal('Unknown Error', 'An unknown error occured while saving. Please try again.');
            }
                            
        }
    }
    request.send(JSON.stringify(config));
   
}

function reload(){
    hideWait();
    fetch('/')
    .then(() => window.location.reload())
    .catch( () => setTimeout(() => {
        reload
    }, 5000))
    ;
}

function restoreFactory(){
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'FactoryReset';
    const confirmed = confirm("Are you sure you want to overwrite all of your settings?");
    if(!confirmed) return;
    showLoading();
    fetch(url)
    .then(() => {
        hideLoading();
        window.location.href = '/';
        
    })
    .catch( () => {
        console.error('Failed to reload after factory restore');
        hideLoading(); //reload
        window.location.reload();
    } );
}


function updateFirmware(){
    var input = document.getElementById("updateFirmware");
    var reader = new FileReader();
    if ("files" in input) {
        if (input.files.length === 0) {
            alert("You did not select a firmware file");
        } else {
            reader.onload = function () {

                if(!input.files[0].name.endsWith(".bin")){
                    showModal('ESP32 Firmware Update Failed', 'File must have a BIN extension!',
                        [
                            {text:'Ok',action: () => { closeModal();} }
                        ]);
                    return;
                }
                
                var writeToDevice = confirm("You are about update the device firmware. Are you sure you want to continue?");
                if(writeToDevice){
                    console.log('uploading new firmware');
                    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
                    const url = base + "/" + 'UpdateFirmware';
                    request.open("POST", url, true);
                    request.setRequestHeader("Content-type", "application/octet-stream");
                    request.onreadystatechange = function () {
                        if (request.readyState == request.DONE) {
                            hideLoading();
                            //hideWait();
                            //hideWait('page');
                            if (request.status == 401) {
                                showModal('<p class="error">' + request.statusText + '</p>', 'Unauthorized');                
                                return;
                            }
                            var response = request.responseText;
                            if(request.status == 200){
                                //pendingChanges = false;
                                showModal('ESP32 Firmware Updated', 'Firmware uploaded. \nRestart device to apply settings? ',
                                [
                                    {text:'No',action: () => { closeModal();} }, 
                                    {
                                        text:'Yes', 
                                        action: () => {
                                            reset(true);closeModal(); 
                                            setTimeout( reload,5000)
                                        }
                                    }

                                ]);
                            } else if(request.status = 500){
                                showModal( 'Firmware update failed!','ESP32 Firmware Update Failed',
                                    [
                                        {text:'Ok',action: () => { closeModal();} }
                                    ]);
                            }
                                            
                        }
                    }
                    
                    request.send(reader.result);
                    showWait();
                }
                
            };
            reader.readAsArrayBuffer(input.files[0]);
        }
    }
}

function restoreSettings() {
    var input = document.getElementById("restoreSettings");
    var reader = new FileReader();
    if ("files" in input) {
        if (input.files.length === 0) {
            alert("You did not select any file to restore");
        } else {
            reader.onload = function () {
                var writeToDevice = confirm("You are about to overwrite your device settings. Are you sure you want to continue?");
                if(writeToDevice){
                    console.log('calling backend to restore from backup ', reader.result);
                    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
                    const url = base + "/" + 'Restore';
                    request.open("POST", url, true);
                    request.setRequestHeader("Content-type", "application/octet-stream");
                    request.onreadystatechange = function () {
                        if (request.readyState == request.DONE) {
                            hideLoading();
                            //hideWait('page');
                            if (request.status == 401) {
                                showModal('<p>Error Occured: Unauthorized </p><p class="error">' + request.statusText + '</p>', 'Unauthorized');                
                                return;
                            }
                            var response = request.responseText;
                            if(request.status == 200){
                                pendingChanges = false;
                                showModal('ESP32 Settings Saved', 'Settings restored sucessfully. \nRestart device to apply settings? ',
                                [
                                    {text:'No',action: () => { closeModal();} }, 
                                    {
                                        text:'Yes', 
                                        action: () => {
                                            reset(true);closeModal(); 
                                            setTimeout( reload,5000)
                                        }
                                    }

                                ]);
                            }
                                            
                        }
                    }
                    request.send(reader.result);
                //     }
                        
                }
            };
            reader.readAsArrayBuffer(input.files[0]);
        }
    }
}

function prettyBytes(size){
    if (size === undefined) return "";
    return size > 1024*1024*1024 ? (size / (1024*1024*1024)).toFixed(2) + "GB" :
        size > 1024*1024 ? (size / (1024*1024)).toFixed(2) + "MB" :
        size > 1024 ? (size / (1024)).toFixed(2) + "KB" :
        size.toFixed(2) + "B"
}

//load drives
var drives;
function loadDrives(){
    xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = () =>{
    
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200)
            {
                drives = JSON.parse(xmlHttp.responseText);
                if(drives.length == 0) return;
                const driveSelectorElement = document.getElementById('logging-location');
                if(driveSelectorElement === null || driveSelectorElement === undefined) return;
                driveSelectorElement.innerHTML = '';
                for(var driveConfig of drives){
                    const driveOptionElement = document.createElement('option');
                    const driveSize =  prettyBytes(driveConfig.size);
                    driveOptionElement.value = driveConfig.index;
                    driveOptionElement.text =  driveConfig.name + " - "  + driveSize;
                    driveOptionElement.tag = driveConfig;
                    driveSelectorElement.appendChild(driveOptionElement);
                }
                if(driveSelectorElement !== null) driveSelectorElement.value = activeConfig.system.logging.location;
                showLoggingProjections();
            }
        }        
    };
    xmlHttp.open("GET", "/esp32_system_info/Disks", true);
    xmlHttp.send(null);
}

loadDrives();

function invalidateOnChange(componentId){
    const element = document.getElementById(componentId);
    if(element === null) return;
    element.addEventListener('change', () => {pendingChanges = true});
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
    const loggingFrequencyElement = document.getElementById('device-logging-frequency');
    if(loggingFrequencyElement !== null){
        loggingFrequencyElement.addEventListener('change',(event) => showLoggingProjections());
    }
    const loggingLocationElement = document.getElementById('logging-location');
    if(loggingLocationElement !== null)
        loggingLocationElement.addEventListener('change', ()=> showLoggingProjections());

    

    if(configDataSting === "$_CONFIGURATION_DATA")
        persistedConfig = {};
    else 
        persistedConfig = JSON.parse(configDataSting);

    if(persistedConfig.type === undefined){
        persistedConfig.type = 'esp32-config';
    }

    // if(persistedConfig.devices === undefined){
    //     persistedConfig.devices = [];
    // }

    if(persistedConfig.wifi === undefined || persistedConfig.wifi.mode === undefined){
        persistedConfig.wifi = {};
        persistedConfig.wifi.mode = 'client';
        persistedConfig.wifi.dhcp = true;
        persistedConfig.preferences = {};
        persistedConfig.preferences.lastPage = 'wifi';   
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
    if(persistedConfig.system.logging === undefined){
        persistedConfig.system.logging = {};
        persistedConfig.system.logging.frequency = 5 * 60; //default to logging every 5 minutes
        persistedConfig.system.logging.retention = 7; //default retention to 1 week
        persistedConfig.system.logging.level = 2; //default to logging errors, warnings, and info        
        persistedConfig.system.logging.location = 0;
    }
    if(persistedConfig.system.mqtt === undefined){
        persistedConfig.system.mqtt = {};
        persistedConfig.system.mqtt.enabled = false;
        persistedConfig.system.mqtt.broker = 'test.mosquitto.org';
        persistedConfig.system.mqtt.port = 8883;
    }

    if(persistedConfig.system.features === undefined){
        persistedConfig.system.features = {};
        persistedConfig.system.features.enableEditor = true;
        persistedConfig.system.features.enableFingerprints = false;
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
    invalidateOnChange('host-name');
    invalidateOnChange('host-enable-ssl');
    invalidateOnChange('host-enable-mdns');
    invalidateOnChange('host-name');

    invalidateOnChange('enable-editor');
    invalidateOnChange('enable-fingerprints');

    invalidateOnChange('network-mode');
    invalidateOnChange('wifi-network');
    invalidateOnChange('wifi-network-password');
    invalidateOnChange('wifi-network-dhcp-yes');
    invalidateOnChange('wifi-network-dhcp-no');
    
    invalidateOnChange('wifi-network-name');
    invalidateOnChange('wifi-network-ap-password');
    invalidateOnChange('wifi-network-ap-ip');
    invalidateOnChange('wifi-network-ap-subnet');

    invalidateOnChange('mqtt-enabled');
    invalidateOnChange('mqtt-subscribe-enabled');
    invalidateOnChange('mqtt-broker');
    invalidateOnChange('mqtt-port');
    invalidateOnChange('mqtt-insecure');

    invalidateOnChange('device-logging-frequency');
    invalidateOnChange('logging-retention');
    invalidateOnChange('logging-level');
    invalidateOnChange('logging-location');

    invalidateOnChange('ntp-host-name');
    invalidateOnChange('time-zone');
    invalidateOnChange('system-theme');

    invalidateOnChange('disable-wifi-timer');
    invalidateOnChange('restart-after');
    
    invalidateOnChange('certificate-source');

    invalidateOnChange('source-nvs');
    invalidateOnChange('source-spiffs');
    
    pendingChanges = false;    
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
    // pendingChanges = (JSON.stringify( persistedConfig) != JSON.stringify(activeConfig))
    const pendingElement = document.getElementById('pending-changes');
    const saveButtonElement = document.getElementById('save-button');
    if(pendingElement === null) return;
    pendingElement.style.visibility = pendingChanges ? 'visible' : 'hidden';
    saveButtonElement.disabled = !pendingChanges;

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
    esp32_config_init(configurationSting);
    
});
