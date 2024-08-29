const genCertViewModelPath = '/W/M/generate_cert_model.json';

var publicCertFile = undefined;
var privateCertFile = undefined;
var publicSaved = 0, privateSaved = 0, certificateGenerated=0;
function showCertificateAction(){
    

    const selectedAction = document.getElementById('certificate-action').value;
    if(selectedAction == 'upload'){
        openModalWithModel(
            'Upload Certificates', 
            [
                {
                    "Name": "Public Certificate",
                    "Type": "File",
                    "Value": "",
                    "Readonly": false,
                    "Source": "public",
                    "Validation": [
                        {"Type": "Required"}
                    ]
                },
                {
                    "Name": "Private Certificate",
                    "Type": "File",
                    "Value": "",
                    "Readonly": false,
                    "Source": "private",
                    "Validation": [
                        {"Type": "Required"}
                    ]
                }
            ],
            null,null,uploadCertificates
        );
        // //clone form to use in modal
        // publicSaved = 0;
        // privateSaved = 0;
        // const certificateUploadSourceElement = document.getElementById('editor-certificate-upload');
        // if(certificateUploadSourceElement === null) return;
        // const certificateUploadElement = certificateUploadSourceElement.cloneNode(true);
        // certificateUploadElement.setAttribute('modal', certificateUploadElement.id);

        // //wire up change event for file dialogs
        // const publicCertFileElement = certificateUploadElement.querySelector('#public-cert');
        // const privateCertFileElement = certificateUploadElement.querySelector('#private-cert');
        // if(publicCertFileElement === null || privateCertFileElement === null) return;
        // publicCertFileElement.addEventListener('change', () =>{
        //     if(publicCertFileElement.files.length > 0)
        //         publicCertFile = publicCertFileElement.files[0];
        // })

        // privateCertFileElement.addEventListener('change', () =>{
        //     if(privateCertFileElement.files.length > 0)
        //         privateCertFile = privateCertFileElement.files[0];
        // })
        
        // showModalComponent(certificateUploadElement,'Upload a certificate', 
        // [
        //     { text:'Cancel',action: () => { publicCertFile = undefined; privateCertFile = undefined; closeModal();}},
        //     { text: 'Upload', action:  () => { uploadCertificates(); }}
        // ], '800px');
        

    }else if(selectedAction == 'generate'){

        if(publicCertFile === undefined)
            openModal("Generate Certificate", genCertViewModelPath,null,null, generateCertificates);
    }
}
var startTime = undefined;
function uploadCertificates(uploadModel){
    var waitElement = showWait();
    //call backend to save files in temp path.
    //set activeConfig's certificate object to target those paths
    //on form save, if paths exist, backend will move certificates to the configured storage
    if(uploadModel.public === undefined || uploadModel.private === undefined) return;
   

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
    publicFileData.append("file", uploadModel.public, "public.cer");
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
    privateFileData.append("file", uploadModel.private, "private.key");
    privateRequest.send(privateFileData);
    startTime = new Date();
    checkIfUploaded(waitElement);
}

//will run for up to one minute
function checkIfUploaded(waitElement){
    const secondsSinceStarted = (new Date().getTime() - startTime.getTime())/1000;
    if((publicSaved == 0 || privateSaved == 0) && secondsSinceStarted < 30)
    {
        setTimeout( () => checkIfUploaded(waitElement), 1000);
        return;
    }
    if(waitElement !== undefined && waitElement !== null && waitElement.dispatchEvent !== undefined){
        waitElement.dispatchEvent(new Event('close'));
    }
    //if uploaded, user is done
    if(publicSaved > 0 && privateSaved > 0){
        
        pendingChanges = true;
        activeConfig.server.certificates.uploaded = true;
    }
        
}

function generateCertificates(model){
    //set activeConfig certificate object to generate info specified
    //on form save, backend will generate certificate to the configured storage

    var waitElement = showWait();

    generateRequestData = {};
    generateRequestData.device = model.device;
    generateRequestData.company = model.company;
    generateRequestData.from = model.from.split('-').join('') + '000000';
    generateRequestData.to = model.to.split('-').join('') + '000000';  
    
    const base = location.href.endsWith('index') ? location.href.replace('/index','') : location.href;

    //send public
    var generateRequest = new XMLHttpRequest();
    const urlPublic = base + "/" + 'GenerateCertificate';    
    generateRequest.open("POST", urlPublic, true);
    generateRequest.onreadystatechange = function () {
        if (generateRequest.readyState == request.DONE) {
            if(waitElement !== undefined && waitElement !== null && waitElement.dispatchEvent !== undefined){
                waitElement.dispatchEvent(new Event('close'));
        
            }
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