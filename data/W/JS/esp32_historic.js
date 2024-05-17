let devices =[];
let days=[];
let charts = [];

function onLogDateChange(logFile){
    if(logFile=== null || logFile === undefined) return;
    //event.data should be name of log file

    //clear data
    devices.forEach( d=> d.data = []);

    //get data from backend. if loaded, render charts
    var request = new XMLHttpRequest();

    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;
    const url = base + "/" + 'Logs?file=/' + logFile;
    request.open("GET", url, true);
    request.setRequestHeader("Content-type", "application/json");
    request.onreadystatechange = function () {
        if (request.readyState == request.DONE) {
            hideLoading();
            if (request.status == 401) {
                alert('error: ' + request.responseText);
                return;
            }
            var response = request.responseText;
            if(response.length <= 0 || response[0] !== '[') return;
            var dailyLog = JSON.parse(response);
            //log is in format [{"time": "time of transaction", "series":[{"id":1,"value":79.3},{"id":2,"value":false},{"id":3,"value":false}]},...]
            
            //break out into arrays, one for each device
            //doing it this way because number of devices can be different for each frame
            for(const logEntry of dailyLog){
                const entryTime = logEntry.time;

                for(const deviceLogEntry of logEntry.series){                    
                    const device = devices.find(d => d.id == deviceLogEntry.id);
                    
                    const entry = {};
                    entry.x = new Date(entryTime).getTime();
                    entry.y = deviceLogEntry.value;
                    device.data.push(entry);
                }
            }

            drawCharts();
            
        }
    }
    request.send(); 
    showLoading();
}


function drawCharts(){
    for(var chart of charts) chart.destroy();
    charts = [];
    
    const chartsContainerElement = document.getElementById('charts-container');
    //chartsContainerElement.innerHTML = '';
    while (chartsContainerElement.hasChildNodes()) {
        chartsContainerElement.removeChild(chartsContainerElement.lastChild);
    }
    //creating a chart for each device
    for(const device of devices){
        createChartContainer(device.name);
        var r = Math.random() * 255, g = Math.random() * 255, b = Math.random()*255;
        console.log('Building chart for device ' + device.name, device.data);
        const chart = createChart(device.name + '-chart', [
            {
                label: device.name,
                tension: 0.2,
                deviceId: device.id,
                borderColor: 'rgba(' + r + ',' + g + ',' + b + ', 1)',
                backgroundColor: 'rgba(' + r + ',' + g + ',' + b + ', .7)',
                data: device.data,

            }
        ]);

        charts.push(chart);
        
    }
}

document.addEventListener('DOMContentLoaded', () => {
    
    //days = JSON.parse(event.data);
    //get chart data
    const daysDropDownElement = document.getElementById('log-days');
    if(daysDropDownElement === null) return;
    daysDropDownElement.innerHTML = '';
    for(const day of days){
        const opt = document.createElement('option');
        opt.value = day.name;
        opt.textContent = day.name.replace('SNAPSHOT_','').replace('.log','');
        daysDropDownElement.appendChild(opt);
    }
    daysDropDownElement.addEventListener('change',(event) => {
        onLogDateChange(event.currentTarget.value);
    })
    onLogDateChange(daysDropDownElement.value)
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