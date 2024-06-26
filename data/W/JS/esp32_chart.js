function createChart(chartEmementId, chartDatasets, onTicksFormat){
    var ctx = document.getElementById(chartEmementId).getContext('2d');
    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: chartDatasets                
        },
       
        options: {
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'minute',
                        parser: 'luxon'
                        
                    }
                }
            },
            plugins: {
               
                zoom:{
                    pan: {
                        enabled: true,
                        mode: 'x',
                        rangeMax: {
                            x: 400
                        },
                        rangeMin: {
                            x: 0
                        }
                    },
                    zoom: {
                        enabled: true,
                        mode: 'x',
                        wheel: {
                            enabled: true
                        },
                        rangeMax: {
                            x: 20000
                        },
                        rangeMin: {
                            x: 1
                        }
                    }
                }
            }
        }
    });
    return chart;
}


//Create a realtime chart element
// @param chartElementId id of HTML canvas  where to render chart
// @param chartDatasets Dataset array of chart.js datasets
// @onRefresh function to be called on chart refresh `function onRefresh(chart){ chart.data.datasets.forEach(ds => { ds.data.push({x: Date.now(), y: 1})})}`
// @onTicksFormat function to be called to format ticks (Y axis) `function onTicksFormat(value,index,ticks){return value + '%'}`
function createRealtimeChart(chartEmementId, chartDatasets, onRefresh, onTicksFormat){
    var ctx = document.getElementById(chartEmementId).getContext('2d');
    var chart = new Chart(ctx, {
        type: 'line',
        data: {
            datasets: chartDatasets                
        },
        options: {     
            responsive: true,
              
            scales: {
                x: {
                    type: 'realtime',
                    realtime: {
                        delay: 1100,
                        onRefresh: onRefresh
                    }
                },
                y: {
                    ticks:{
                        /* beginAtZero: true, */
                        callback: onTicksFormat                            
                    }
                }
            },
            plugins: {
                streaming: {
                    /*frameRate: 5*/
                    duration: 1000*60*5
                },
                zoom:{
                    pan: {
                        enabled: true,
                        mode: 'x',
                        rangeMax: {
                            x: 400
                        },
                        rangeMin: {
                            x: 0
                        }
                    },
                    zoom: {
                        enabled: true,
                        mode: 'x',
                        wheel: {
                            enabled: true
                        },
                        rangeMax: {
                            x: 20000
                        },
                        rangeMin: {
                            x: 1
                        }
                    }
                }
            }
        }
    });
    return chart;
}