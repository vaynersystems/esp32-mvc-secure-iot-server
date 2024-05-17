let deviceService = new esp32_socket('devices');
let dataAvailable = 0;
let lastLiveDataFrame = null;
let devices =[];

function onSnapshotData(event){
    if(event.data === null || event.data === undefined) return;
    //update chart stream data
    lastLiveDataFrame = JSON.parse(event.data);
    dataAvailable = devices.length;   
}


function updateStatus(){
    const connectionStatusElement = document.getElementById('connection-status');
    if(connectionStatusElement === null) return;
    
    deviceService.send('snapshot'); // request snapshot
    connectionStatusElement.innerHTML = printStateName( deviceService.socket.readyState);

    if(deviceService.socket.readyState == 3) //closed
        deviceService.connect( onSnapshotData);
    setTimeout(updateStatus, 5000);
}

function printStateName(state){
    switch(state){
        case 0:
            return 'Connecting';
            
        case 1:
            return 'Open';
        case 2:
            return 'Closing';
        default:
            return 'Closed';
    }        
}



document.addEventListener('DOMContentLoaded', () => {
    
    lastLiveDataFrame = {};
    deviceService.connect(onSnapshotData);
    setTimeout(updateStatus, 1000);
    console.log(deviceService);


    //creating a chart for each device
    for(const device of devices){
        createChartContainer(device.name);
        var r = Math.random() * 255, g = Math.random() * 255, b = Math.random()*255;

        createRealtimeChart(device.name + '-chart',[
            {
                label: device.name,
                tension: 0.2,
                deviceId: device.id,
                borderColor: 'rgba(' + r + ',' + g + ',' + b + ', 1)',
                backgroundColor: 'rgba(' + r + ',' + g + ',' + b + ', .7)',
            }
        ], onRefresh);
        
    }
    
}, false);

function createChartContainer(deviceName){
    const chartsContainerElement = document.getElementById('charts-container');
    const chartContainerElement = document.createElement('div');
    const chartHeaderElement = document.createElement('div');   
    const chartDetailElement = document.createElement('div');
    const chartElement = document.createElement('canvas');
    chartContainerElement.className = 'grow';
    chartHeaderElement.className = 'header-sub';
    chartHeaderElement.textContent = deviceName;
    chartDetailElement.className = 'details grow';
    chartElement.id = deviceName + '-chart';

    chartDetailElement.appendChild(chartElement);
    chartContainerElement.appendChild(chartHeaderElement);
    chartContainerElement.appendChild(chartDetailElement);
    chartsContainerElement.appendChild(chartContainerElement);
}

function onRefresh(chart) {
    if(deviceService.socket.readyState !== 1) return; //if socket is not open, nothing to do.
    if(dataAvailable == 0) return; //if no new data, nothing to do
    //update data field if available
    chart.data.datasets.forEach(ds => {
        ds.data.push({
            x: Date.now(),
            y: getLastValue(ds.deviceId)    
        })
    });
    if(dataAvailable > 0) dataAvailable--; // keep track of 2 data points
}

function getLastValue(deviceId){
    const frameTime = lastLiveDataFrame["time"];
    const frameDevices = lastLiveDataFrame["series"];
    const device = frameDevices.find(d => d.id == deviceId);
    if(device === undefined) return;
    return device.value;
}
