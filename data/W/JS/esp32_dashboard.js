let deviceService = new esp32_socket('devices');
let dataAvailable = 0;
let lastLiveDataFrame = null;

function onSnapshotData(event){
    if(event.data === null || event.data === undefined) return;
    // {"time":"04/15/2024 09:13:00","series":[{"id":1,"value":76.88749695,"type":"double"},{"id":2,"value":true,"type":"bool"}]}
    //update chart stream data
    lastLiveDataFrame = JSON.parse(event.data);
    dataAvailable = 2;
    //update uptime clock
    updateUptime(lastLiveDataFrame.UPTIME);
    updateStackAndHeap();
}


function updateStatus(){
    const connectionStatusElement = document.getElementById('connection-status');
    if(connectionStatusElement === null) return;
    
    deviceService.send(''); //anything other than ping to this service will return memory usage
    connectionStatusElement.innerHTML = printStateName( deviceService.socket.readyState);

    if(deviceService.socket.readyState == 3) //closed
        deviceService.connect( serverMessage);
    setTimeout(updateStatus, 500);
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
    deviceService.connect(serverMessage);
    setTimeout(updateStatus, 1000);
    console.log(deviceService);

    //initialize charts
    createChart('stack-chart',[
        {
            label: 'Free Stack',
            borderColor: '#F70809',
            borderWidth: 1,
            backgroundColor: 'rgba(247,8,8, .5)',
            field: 'STACK_FREE',
            fill: false,
            tension: 0.2
        },
        {
            label: 'Used Stack',
            borderColor: 'rgb(255, 99, 132)',
            borderWidth: 1,
            backgroundColor: 'rgba(255, 99, 132, .5)',
            field: 'STACK_USED',
            fill: false,
            tension: 0.2
        },
        {
            label: 'Total Stack',
            borderColor: 'rgb(54, 162, 235)',
            borderWidth: 1,
            backgroundColor: 'rgba(54, 162, 235, 0.5)',
            field: 'STACK_TOTAL',
            tension: 0.2
        }
    ], onRefresh, onticksFormatBytes);

    createChart('heap-chart',[
        {
            label: 'Free Heap',
            borderColor: 'rgba(127,124,210, 1)',
            backgroundColor: 'rgba(127,124,210, .5)',
            field: 'HEAP_FREE',
            tension: 0.2
        },
        {
            label: 'Used Heap',
            borderColor: 'rgb(223, 194, 32)',
            backgroundColor: 'rgba(223, 194, 32, 0.75)',
            field: 'HEAP_USED',
            tension: 0.2
        },
        {
            label: 'Total Heap',
            borderColor: 'rgb(167, 162, 235)',
            backgroundColor: 'rgba(167, 162, 235, 0.5)',
            field: 'HEAP_TOTAL',
            tension: 0.2
        }
    ], onRefresh, onticksFormatBytes);

    
}, false);


function onRefresh(chart) {
    if(deviceService.socket.readyState !== 1) return; //if socket is not open, nothing to do.
    if(dataAvailable == 0) return; //if no new data, nothing to do
    //update data field if available
    chart.data.datasets.forEach(ds => {
        ds.data.push({
            x: Date.now(),
            y: lastLiveDataFrame[ds.field]    
        })
    });
    if(dataAvailable > 0) dataAvailable--; // keep track of 2 data points
}

function onticksFormatBytes(value,index,ticks){        
    return formatByte(value);
}

function formatByte(value){
    if(value/1024/1024 > 1.05)
        return Math.round((value/1024/1024) * 100) / 100 + ' MB';
    if(value/1024 > 1.05)
        return Math.round((value/1024) * 100) / 100 + ' KB';
    return value + ' B';
}