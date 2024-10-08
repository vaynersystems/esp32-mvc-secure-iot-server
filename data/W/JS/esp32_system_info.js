let persistanceSocket = new esp32_socket('status');
let dataAvailable = 0;
let lastLiveDataFrame = null;

function serverMessage(event){
    if(event.data === null || event.data === undefined) return;
    if(event.data == 'pong') return; 
    
    //update chart stream data
    lastLiveDataFrame = JSON.parse(event.data);
    dataAvailable = 3;
    //update uptime clock
    updateUptime(lastLiveDataFrame.UPTIME);
    updateStackAndHeap();
}

function updateStackAndHeap(){
    if(lastLiveDataFrame == null) return;

    const stackUsedElement = document.getElementById('stack-used');
    const stackFreeElement = document.getElementById('stack-free');
    const stackTotalElement = document.getElementById('stack-total');
    const stackPercentUsedElement = document.getElementById('stack-percent-used');
    const stackPercentUsedBarElement = document.getElementById('stack-percent-used-bar');
    
    const heapUsedElement = document.getElementById('heap-used');
    const heapFreeElement = document.getElementById('heap-free');
    const heapTotalElement = document.getElementById('heap-total');
    const heapPercentUsedElement = document.getElementById('heap-percent-used');
    const heapPercentUsedBarElement = document.getElementById('heap-percent-used-bar');

    const psramUsedElement = document.getElementById('psram-used');
    const psramFreeElement = document.getElementById('psram-free');
    const psramTotalElement = document.getElementById('psram-total');
    const psramPercentUsedElement = document.getElementById('psram-percent-used');
    const psramPercentUsedBarElement = document.getElementById('psram-percent-used-bar');

    if(stackUsedElement !== null) stackUsedElement.textContent = formatByte(lastLiveDataFrame.STACK_USED);
    if(stackFreeElement !== null) stackFreeElement.textContent = formatByte(lastLiveDataFrame.STACK_FREE);
    if(stackTotalElement !== null) stackTotalElement.textContent = formatByte(lastLiveDataFrame.STACK_TOTAL);
    if(stackPercentUsedElement !== null) stackPercentUsedElement.textContent = lastLiveDataFrame.STACK_PERCENT_USED + '.0%';
    if(stackPercentUsedBarElement !== null) stackPercentUsedBarElement.style.setProperty('--percent', lastLiveDataFrame.STACK_PERCENT_USED);

    if(heapUsedElement !== null) heapUsedElement.textContent = formatByte(lastLiveDataFrame.HEAP_USED);
    if(heapFreeElement !== null) heapFreeElement.textContent = formatByte(lastLiveDataFrame.HEAP_FREE);
    if(heapTotalElement !== null) heapTotalElement.textContent = formatByte(lastLiveDataFrame.HEAP_TOTAL);
    if(heapPercentUsedElement !== null) heapPercentUsedElement.textContent = lastLiveDataFrame.HEAP_PERCENT_USED + '.0%';
    if(heapPercentUsedBarElement !== null) heapPercentUsedBarElement.style.setProperty('--percent', lastLiveDataFrame.HEAP_PERCENT_USED);        
    
    if(psramUsedElement !== null) psramUsedElement.textContent = formatByte(lastLiveDataFrame.PSRAM_USED);
    if(psramFreeElement !== null) psramFreeElement.textContent = formatByte(lastLiveDataFrame.PSRAM_FREE);
    if(psramTotalElement !== null) psramTotalElement.textContent = formatByte(lastLiveDataFrame.PSRAM_TOTAL);
    if(psramPercentUsedElement !== null) psramPercentUsedElement.textContent = lastLiveDataFrame.PSRAM_PERCENT_USED + '.0%';
    if(psramPercentUsedBarElement !== null) psramPercentUsedBarElement.style.setProperty('--percent', lastLiveDataFrame.PSRAM_PERCENT_USED);        
}

function updateUptime(seconds){
    const uptimeElement = document.getElementById('uptime');
    var timeLeft = seconds;
    if(uptimeElement === null) return;
    
    var partHours=0, partMinutes=0, partSecods = 0;
    if(timeLeft / 3600 > 1){
        partHours = Math.floor(timeLeft / 3600);
        timeLeft -= partHours * 3600;
    }
    if(timeLeft / 60 > 1){
        partMinutes = Math.floor(timeLeft / 60);
        timeLeft -= partMinutes * 60;
    }
    partSeconds = timeLeft;

    // uptimeElement.textContent = (partHours > 0 ? partHours + (partHours > 1 ? ' hours ' : ' hour ') : '') +
    //     (partMinutes > 0 ? partMinutes + (partMinutes > 1 ? ' minutes ' : ' minute ') : '') +
    //     (partSeconds > 0 ? partSeconds + (partSeconds > 1 ? ' seconds ' : ' second ') : '');
    uptimeElement.textContent = partHours.toString().padStart(2,'0') + ':' + partMinutes.toString().padStart(2,'0') + ':' + partSeconds.toString().padStart(2,'0');
}

function updateStatus(){
    const connectionStatusElement = document.getElementById('connection-status');
    if(connectionStatusElement === null) return;
    
    persistanceSocket.send(''); //anything other than ping to this service will return memory usage
    connectionStatusElement.innerHTML = printStateName( persistanceSocket.socket.readyState);

    if(persistanceSocket.socket.readyState == 3) //closed
        persistanceSocket.connect( serverMessage);
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
    persistanceSocket.connect(serverMessage);
    setTimeout(updateStatus, 1000);
    console.log(persistanceSocket);

    //initialize charts
    createRealtimeChart('stack-chart',[
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

    createRealtimeChart('heap-chart',[
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

    createRealtimeChart('psram-chart',[
        {
            label: 'Free PSRAM',
            borderColor: 'rgba(127,124,210, 1)',
            backgroundColor: 'rgba(127,124,210, .5)',
            field: 'PSRAM_FREE',
            tension: 0.2
        },
        {
            label: 'Used PSRAM',
            borderColor: 'rgb(223, 194, 32)',
            backgroundColor: 'rgba(223, 194, 32, 0.75)',
            field: 'PSRAM_USED',
            tension: 0.2
        },
        {
            label: 'Total PSRAM',
            borderColor: 'rgb(167, 162, 235)',
            backgroundColor: 'rgba(167, 162, 235, 0.5)',
            field: 'PSRAM_TOTAL',
            tension: 0.2
        }
    ], onRefresh, onticksFormatBytes);

    
}, false);


function onRefresh(chart) {
    if(persistanceSocket.socket.readyState !== 1) return; //if socket is not open, nothing to do.
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