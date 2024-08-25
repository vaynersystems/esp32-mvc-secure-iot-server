const genCertViewModelPath = '/W/M/generate_cert_model.json';

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
        // certificateGenerated = 0;
        // const certificateGenerateSourceElement = document.getElementById('editor-certificate-generate');
        // if(certificateGenerateSourceElement === null) return;
        // const certificateGenerateElement = certificateGenerateSourceElement.cloneNode(true);
        // certificateGenerateElement.setAttribute('modal', certificateGenerateElement.id);

        if(publicCertFile === undefined)
            openModal("Generate Certificate", genCertViewModelPath,null,null, generateCertificates)
        
        // showModalComponent(certificateGenerateElement,'Generate a new certificate', 
        // [
        //     { text:'Cancel',action: () => { closeModal();}},
        //     { text: 'Generate', action:  () => { generateCertificates();  }}
        // ]);
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