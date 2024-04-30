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
        console.log(child.tagName);
    }
    contentElement.appendChild(sectionElement);
    selectedSection = section;
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
        networkPasswordElement.innerHTML = networkPassword.length === 0 ? '' : '*'.repeat(networkPassword.length);
    }
    
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
            if(response.length <= 0) return;
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

document.getElementById('network-mode').addEventListener('change', function(){
    const modeSelectorElement = document.getElementById('network-mode');

    if(modeSelectorElement === undefined) return;
    selectWifiMode(modeSelectorElement.value);
})
/* END OF WIFI */


function loadSettings(configData){
    activeConfig = configData;
    
    //overall
    var view = configData.preferences === undefined ? 'wifi' : configData.preferences.lastPage;
    selectView(view);

    //wifi
    selectWifiMode(configData.wifi.mode);
    selectDHCPMode(configData);
    //system
    const hostNameElement = document.getElementById('host-name');
    const ntpServerElement = document.getElementById('ntp-host-name');
    const timeZoneElement = document.getElementById('time-zone');

    if(hostNameElement !== null) hostNameElement.value = configData.system.hostname;
    if(ntpServerElement !== null) ntpServerElement.value = configData.system.ntp.server;
    if(timeZoneElement !== null) timeZoneElement.value = configData.system.ntp.timezone;

    //server
    const disableWifiElement = document.getElementById('disable-wifi-timer');
    if(disableWifiElement !== null) disableWifiElement.value = configData.system.disableWifiTimer;
}
function seveSettingsFromForm(){
    var config = persistedConfig;
    //devices
    if(config.devices === undefined)
        config.devices = [];

    //schedules
    if(config.schedules === undefined)
        config.system = [];

    //system
    if(config.system === undefined)
        config.system = {};
    config.system.hostname = document.getElementById('host-name').value;
    config.system.NTPServer = document.getElementById('ntp-host-name').value;
    config.system.timezone = document.getElementById('time-zone').value;

    //server
    if(config.server === undefined)
        config.server = {};
    config.server.disableWifiTimer = document.getElementById('disable-wifi-timer').value;

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
        config.wifi.ap.ip = document.getElementById('wifi-network-ap-ip')?.value;
        config.wifi.ap.subnet = document.getElementById('wifi-network-ap-subnet')?.value;            
    }
    config.type = 'esp32-config'
    saveSettings(config);
}
//parse form, collecting all of the input, provide as json to post
function saveSettings(config){
    //TODO: bind controls to activeConfiguration model, just persist it here
    

    console.log(config);

    const url = location.href + "/" + 'SaveConfigData'
    request.open("POST", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {

            hideWait('page');
            if (request.status == 401) {
                document.getElementById('error').appendChild('<p class="error">' + request.responseText + '</p>');
                return;
            }
            var response = request.responseText;
            if(request.status == 200){
                showModal('Settings saved sucessfully. \nRestart device to apply settings? ','ESP32 Settings Saved', [{text:'No'}, {text:'Yes', action:() => {reset()}}]);
            }
                            
        }
    }
    request.send(JSON.stringify(config));
    showWait('page');
}

function showWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== null && waitElement !== null) waitElement.style.display = 'block';
}

function hideWait(which){
    const waitElement = document.getElementById(which + '-wait');
    if(waitElement !== undefined && waitElement !== null ) waitElement.style.display = 'none'; //hide wait indicator
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
                    // var x = confirm("File seems to be valid, do you wish to continue?");
                    // if (x) {          
                    //     loadSettings(JSON.parse(json))
                    //     showModal("Settings restored to the configuration screen. Review the settings and save to persist to device.","Restoring from backup");
                    // }
                    var writeToDevice = confirm("You are about to overwrite your device settings. Are you sure you want to continue?");
                    if(writeToDevice)
                        saveSettings(JSON.parse(json));
                }
            };
            reader.readAsText(input.files[0]);
        }
    }
}


function esp32_config_init(configDataSting){
    const links = document.getElementsByTagName('li');
    for(let link of links){
        link.addEventListener('click', (e) => {selectView(link.id)});
    }
    const dhcpOptions = document.getElementsByName('wifi-network-dhcp');
    for(let opt of dhcpOptions){
        opt.addEventListener('change', (e) => { selectDHCPMode(e.target.value); })
    }
    if(configDataSting === "$_CONFIGURATION_DATA")
        persistedConfig = {};
    else 
        persistedConfig = JSON.parse(configDataSting);

    if(persistedConfig.type === undefined){
        persistedConfig.type = 'esp32-config';
    }
    if(persistedConfig.wifi === undefined || persistedConfig.wifi.mode === undefined){
        persistedConfig = {};
        persistedConfig.wifi = {};
        persistedConfig.wifi.mode = 'client';
        persistedConfig.wifi.dhcp = true;
        persistedConfig.preferences = {};

        persistedConfig.preferences.lastPage = 'wifi';   
    }
    if(persistedConfig.system === undefined){
        persistedConfig.system = {};
        var d = new Date().getDay();
        var m = new Date().getMonth();
        var y = new Date().getFullYear();
        persistedConfig.system.hostname = 'ESP32 Host' + y + "-" + m + "-" + d; 
    }
    if(persistedConfig.system.ntp === undefined){
        persistedConfig.system.ntp = {};
        persistedConfig.system.ntp.server = '0.us.pool.ntp.org';
        persistedConfig.system.ntp.timezone = '-5';
    }

    if(persistedConfig.server === undefined)
        persistedConfig.server = {};

    if(persistedConfig.server.disableWifiTimer === undefined){
        persistedConfig.server.disableWifiTimer = 'never'
    }

    loadSettings(persistedConfig)
    
}


//should be moved to a global file

function reset(){
    if(confirm("Are you sure you want to restart the device?")){
        const url = base + "/" + 'ResetDevice'
        request.open("POST", url, true);
        request.send(null);
    }
}

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


    
