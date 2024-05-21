let logFiles=[];
var grid = undefined;
function onLogFileChange(logFile){
    if(logFile=== null || logFile === undefined) return;

    const logsContainerElement = document.getElementById("logs-container");
    if(logsContainerElement === null) return;
    while(logsContainerElement.hasChildNodes())
        logsContainerElement.removeChild(logsContainerElement.lastChild);
    //logsContainerElement.innerHTML = '';
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
            var response = request.responseText.replace(/(?:\r\n|\r|\n|\t)/g, '');
            if(response.length <= 0 || response[0] !== '[') return;
            var logData = JSON.parse(response);
            if(grid === undefined){
                grid = new gridjs.Grid({
                    columns: ['Time', 'Type', 'Message'],
                    search: true,
                    sort: true,
                    pagination: {
                        limit: 10
                    },
                    data: logData
                });
                grid.render(logsContainerElement);
            } else{
                grid.updateConfig({
                    data: logData
                }).forceRender();
            }
            
            
        }
    }
    request.send(); 
    showLoading();
}

document.addEventListener('DOMContentLoaded', () => {
    
    //days = JSON.parse(event.data);
    //get chart data
    const logFilesDropDownElement = document.getElementById('log-files');
    if(logFilesDropDownElement === null) return;
    logFilesDropDownElement.innerHTML = '';
    for(const logfile of logFiles.sort((a,b) => {if(a.name < b.name) return -1; if(b.name < a.name) return 1; return 0;})){
        const opt = document.createElement('option');
        opt.value = logfile.name;
        opt.textContent = logfile.name;
        logFilesDropDownElement.appendChild(opt);
    }
    logFilesDropDownElement.addEventListener('change',(event) => {
        onLogFileChange(event.currentTarget.value);
    })
    onLogFileChange(logFilesDropDownElement.value)
}, false);