#ifndef _ESP32_ROUTER_CPP
#define _ESP32_ROUTER_CPP
#include "esp32_router.h"
#include "../Config.h"
#include "../CORE/esp32_server.h"
#include <string_extensions.h>

extern esp32_server server;

void esp32_router::InitConfigurableRouting()
{
    
    
}

void esp32_router::RegisterHandler(ResourceNode* resourceNode) {
    RegisterHandler( resourceNode->_path.c_str(), resourceNode->getMethod().c_str(), resourceNode->_callback);
}

void esp32_router::RegisterHandler(String nodeMapPath, HTTPMETHOD method, HTTPSCallbackFunction* handler) {
    String methodName;
    switch (method) {
    case HTTPMETHOD_GET:
        methodName = "GET";
        break;
    case HTTPMETHOD_POST:
        methodName = "POST";
        break;
    case HTTPMETHOD_DELETE:
        methodName = "DELETE";
        break;
    case HTTPMETHOD_PUT:
        methodName = "PUT";
        break;
    }
    RegisterHandler( nodeMapPath, methodName, handler);
}

void esp32_router::RegisterHandlers(fs::FS& fs, const char* dirname, uint8_t levels) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            if (levels) { //print out directory contents to serial
                esp32_fileio::listDir(fs, &Serial, file.name(), levels - 1);
            }
        }
        else {  //Register file handler as filename         

            String mappingPath = String(file.name());
            mappingPath.remove(0, sizeof(SITE_ROOT) - 1);

            RegisterHandler( mappingPath.c_str(), HTTPMETHOD_GET, &handleFile);


            int startIdx = mappingPath.lastIndexOf('/');
            if (startIdx > 0) {
                String filePath = mappingPath.substring(startIdx);
                if (filePath.length() > 0)
                    RegisterHandler( filePath.c_str(), HTTPMETHOD_GET, &handleFile);
            }

            //if gz file, register handler without gz
            if (String(file.name()).endsWith(".gz")) {
                String nonGZName = String(file.name());
                nonGZName.remove(nonGZName.length() - 3);

                RegisterHandler( nonGZName.c_str(), HTTPMETHOD_GET, &handleFile);

                nonGZName.remove(0, sizeof(SITE_ROOT) - 1);
               //Serial.printf(" Resistering GZ Handler[%s , %s]\n", file.name(), nonGZName.c_str());
                RegisterHandler( nonGZName.c_str(), HTTPMETHOD_GET, &handleFile);                
            }
        }
        file = root.openNextFile();
    }
}


void esp32_router::RegisterHandler(String nodeMapPath, String method, HTTPSCallbackFunction* handler) {
    ResourceNode* node = new ResourceNode(nodeMapPath.c_str(), method.c_str(), handler);
    server.secureServer->registerNode(node);
    server.unsecureServer->registerNode(node);    
}


void esp32_router::handleCORS(HTTPRequest* req, HTTPResponse* res) {
    Serial.println(">> CORS here...");
    res->setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->setHeader("Access-Control-Allow-Origin", "*");
    res->setHeader("Access-Control-Allow-Headers", "*");
}

void esp32_router::handleFileList(HTTPRequest* req, HTTPResponse* res) {
    std::string dir,path, filter;
    req->getParams()->getQueryParameter("dir", dir);
    req->getParams()->getQueryParameter("path", path);
    req->getParams()->getQueryParameter("filter", filter);
    
    res->setHeader("Content-Type", "application/json");
    if (path.length() > 0) {
        //search path
        if(path.length() > 1 && path[path.length() - 1] == '/')
            path = path.substr(0,path.length() - 1);

        std::list<std::string> files = std::list<std::string>();
        
        Serial.printf("Searching for files only in path %s\n", path.c_str());
        esp32_fileio::buildOrderedFileList(SPIFFS, path.c_str(), filter.c_str(), 3, &files,false);
        esp32_fileio::printFileSearchOrdered(res, &files,filter);
    }
    else {
        //print dir recursivly
        if (dir.length() <= 0) 
        {
            dir = "/";
            Serial.printf("Empty path, starting from %s\n", dir.c_str());
        }
        
        std::list<std::string> files = std::list<std::string>();
        esp32_fileio::buildOrderedFileList(SPIFFS, dir.c_str(), filter.c_str(), 3, &files);
        esp32_fileio::printDirOrdered(res, &files);
    }
}

int esp32_router::handlePagePart_Title(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{
    if(content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_TITLE, (req->getHeader(HEADER_AUTH).length() > 0) ? "/T/title_int.html" : "/T/title_pub.html");
    
    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_TITLE, content);
   
}

int esp32_router::handlePagePart_Head(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_HEAD, "/T/head.html");  

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_HEAD, content);
}

int esp32_router::handlePagePart_Header(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_HEADER, (req->getHeader(HEADER_GROUP).length() > 0) ? "/T/header_int.html" : "/T/header_pub.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_HEADER, content);
}
int esp32_router::handlePagePart_Menu(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_MENU, (req->getHeader(HEADER_GROUP).length() > 0) ? "/T/menu_int.html" : "/T/menu_pub.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_MENU, content);
}

int esp32_router::handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{
    if (content.length() <= 0) {
        int idx = line.indexOf(HTML_REF_CONST_CONTENT);
        if (idx < 0) return idx;
        res->print(line.substring(0, idx));
        handleFile(req, res);

        res->println(line.substring(idx + sizeof(HTML_REF_CONST_CONTENT) - 1));

        return idx;
    }

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_CONTENT, content);    
}


int esp32_router::handlePagePart_Content(HTTPRequest* req, HTTPResponse* res, String line, Base_Controller * controller)
{
    int idx = line.indexOf(HTML_REF_CONST_CONTENT);
    if (idx < 0) return idx;
    res->print(line.substring(0, idx));
    if (!controller->controllerTemplate.RenderTemplate(req, res))
        handle404(req, res);//if not able to render template, handle with 404
    res->println(line.substring(idx + sizeof(HTML_REF_CONST_CONTENT) - 1));

    return idx;
}

int esp32_router::handlePagePart_Footer(HTTPRequest* req, HTTPResponse* res, String line, std::string content = "")
{   
    if (content.length() <= 0) {
        int idx = line.indexOf(HTML_REF_CONST_FOOTER);
        if (idx < 0) return idx;
        res->print(line.substring(0, idx));

        auto footerModule = /*controllerFactory->hasInstance(route.controller) ? */
            controllerFactory->createInstance("_footer", "index");// : NULL;

        if (footerModule != NULL) {
            //module found
            footerModule->Action(req, res); //execute module action

            //render module output
            if (!footerModule->controllerTemplate.RenderTemplate(req, res))
                HTTPS_LOGE("Failure to render footer template");     

            res->println(line.substring(idx + sizeof(HTML_REF_CONST_FOOTER) - 1));
            return idx;
                
        }
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_FOOTER, "/W/T/V/_footer.html");
    }    

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_FOOTER, content);
}

int esp32_router::handlePagePart_FromFile(HTTPRequest* req, HTTPResponse* res, String line, const char * searchString, String fileName){
    
    int idx = line.indexOf(searchString);
    if (idx >= 0) {
        String fFileName = SITE_ROOT;
        fFileName += fileName;

        Serial.printf("\t[PagePart Parser]. Found %s in %s. Filling from %s. \n", searchString, line.c_str(), fFileName.c_str());
        res->print(line.substring(0, idx).c_str());
        
        //Serial.printf("[PREFIX] %s\n", line.substring(0, idx).c_str());

        if (SPIFFS.exists(fFileName.c_str())) {
            File fPagePart = SPIFFS.open(fFileName);

            while (fPagePart.available()) {
                String docLine = fPagePart.readStringUntil('\n');
                res->print(docLine.c_str());
                if (fPagePart.available()) res->print('\n'); //write new line if not last line
                //Serial.printf("[CONTENT] %s\n", docLine.c_str());
            }
            fPagePart.close();

        }
        else
        {
            Serial.printf("File %s not found.\n", fFileName.c_str());
            res->printf("Page Part %s file %s NOT FOUND!",searchString,fFileName.c_str());
        }
        res->println(line.substring(idx + strlen(searchString)).c_str());
        //Serial.printf("[SUFFIX] %s\n\n", line.substring(idx + strlen(searchString)).c_str());
    }
    

    return idx;// > 0 ? idx : 0;
}
int esp32_router::handlePagePart_FromString(HTTPRequest* req, HTTPResponse* res, String line, const char* searchString, std::string content) {

    int idx = line.indexOf(searchString);
    if (idx >= 0) {


        //Serial.printf("[PagePart Parser]. Found %s in %s. Filling from string. ", searchString, line.c_str());
        res->print(line.substring(0, idx).c_str());

       // Serial.printf("[PREFIX] %s\n", line.substring(0, idx).c_str());
        res->print(content.c_str());
        //Serial.printf("[CONTENT] %s\n", content.c_str());

        res->println(line.substring(idx + strlen(searchString)).c_str());
        //Serial.printf("[SUFFIX] %s\n\n", line.substring(idx + strlen(searchString)).c_str());
    }

    return idx;// > 0 ? idx : 0;
}
//Get the route information for a controler/action syntax. returns true if controller found, false otherwise
bool esp32_router::GetControllerRoute(HTTPRequest* request, esp32_controller_route& routeObj)
{

    std::string controller(""), action("");
    std::string pathString(""), queryString("");
    pathString = request->getRequestString();
    /*
        If request is not a get, route to the proper action
        Otherwise, decode action from url
    */

    int queryStart = pathString.find_first_of('?');
    if (queryStart > 0)
    {
        queryString = pathString.substr(queryStart + 1);
        pathString = pathString.substr(0,queryStart);
        //Serial.printf("Query string detected with %s path and %s query", pathString.c_str(), queryString.c_str());
        //reqString.erase(queryStart);
    }
    if (pathString.find_first_of('/') == 0) //remove leading slash
        pathString.erase(0, 1);

    //default controller if none is specified
    controller = "invalid";//"esp32_home";
    if(strstr(pathString.c_str(),"index.html") != nullptr || pathString.length() == 0)
        controller = "esp32_home";
    else controller = pathString;
    //default action
    action = "index";
    //explicit action defined
    int reqIdxSlash = pathString.find_first_of('/');
    if (reqIdxSlash > 0)
    {
        controller = pathString.substr(0, reqIdxSlash);
        pathString.erase(0, reqIdxSlash + 1);
        action = pathString;
    }

    if(strcmp(request->getMethod().c_str(),"GET") != 0){
        action = request->getMethod();
    }else {
        reqIdxSlash = pathString.find_first_of('/');
        if (reqIdxSlash > 0)
        {
            action = pathString.substr(0, reqIdxSlash);
            pathString.erase(0, reqIdxSlash + 1);
        }
        else {
            action = pathString;
            pathString.erase(0);
        }
    }
    Serial.printf("Parsed url. Controller=%s Action=%s Remainder=%s Query=%s\n", controller.c_str(), action.c_str(), pathString.c_str(), queryString.c_str());
    
    routeObj.action = action;
    routeObj.controller = controller;
    routeObj.params = urlDecode(queryString);
    //return IsValidRoute(routeObj);
    return controllerFactory->hasInstance(controller.c_str());
    
}

bool esp32_router::IsValidRoute(esp32_controller_route & route){
    int numOfControllers = BaseFactory::getInstanceCount();
    //Serial.printf("Found %i controllers\n", numOfControllers);
    for(int i=0;i< numOfControllers;i++){
        auto controller = BaseFactory::getInstanceAt(i);        
        //if(controller.first[0] == '_') continue;

        vector<string> actions = {};
        controller.second()->GetActions(&actions);
        //Serial.printf("Controller %d of %d: %s with %i actions\n",i, numOfControllers,controller.first.c_str(), actions.size());
        for(const string& action : actions) {            
            if(route.controller.length() == controller.first.length() && route.action.length() == action.length()){                
                if(iequals(route.controller.c_str(),controller.first.c_str(),route.controller.length())
                    && iequals(route.action.c_str(), action.c_str(),action.length())
                ){
                    Serial.printf("Matched route controller %s action %s against cataloged controller %s action %s\n", route.controller.c_str(), route.action.c_str(),controller.first.c_str(), action.c_str());
                    //case correct action
                    route.action = action;
                    return true;
                }
            }
        }
    }
   return false;
}
void esp32_router::handleFileUpload(HTTPRequest* req, HTTPResponse* res) {
    if (req->getMethod() == "DELETE") {
        res->println("<html><head><title>File Deleted</title><head><body><h1>File Deleted</h1>");

        HTTPURLEncodedBodyParser parser(req);
        std::string filename, filePath;
        bool savedFile = false;
        while (parser.nextField()) {
            std::string name = parser.getFieldName();
            Serial.printf("Parsing field %s\n", name.c_str());
            if (name.substr(0, strlen("------WebKitFormBoundary")).compare("------WebKitFormBoundary") == 0) {
                char buf[512];
                size_t readLength = parser.read((byte*)buf, 512);
                filePath = std::string(buf, readLength);
                Serial.printf("Parsing substring from idx %d to idx %d\n", filePath.find_first_of("://"), filePath.find_first_of("------"));
                filePath = filePath.erase(0, filePath.find_first_of("://"));
                filePath = filePath.erase(filePath.find_first_of("------") - 2);
                Serial.printf("Received request to delete %s.\n", filePath.c_str());

                filename = filePath.substr(filePath.find_first_of('?') + 1);
                if (filename == "") {
                    res->println("<p>Error: form contained content before filename.</p>");
                    break;
                }
                if (!SPIFFS.exists(filename.c_str())) {
                    res->printf("File [%s] with at path %s not found.", filename.c_str(), filePath.c_str());
                    //break;
                }
                SPIFFS.remove(filename.c_str());
                res->printf("<p>Deleted %s</p>", filename.c_str());
                savedFile = true;
            }
            else {
                Serial.printf("Expected string %s. Instead found %s\n", "------WebKitFormBoundary", name.substr(0, strlen("------WebKitFormBoundary")).c_str());
            }
            /* else {
                 res->printf("<p>Unexpected field %s</p>", name.c_str());
             }*/
        }
        if (!savedFile) {
            res->println("<p>No file to delete...</p>");
        }
        res->println("</body></html>");
        return;
    }
    else if (req->getMethod() == "GET") {}

    res->println("<html><head><title>File Edited</title><head><body><h1>File Edited</h1>");
    HTTPMultipartBodyParser* parser;
    std::string contentType = req->getHeader("Content-Type");
    size_t semicolonPos = contentType.find(";");
    bool savedFile = false;
    if (semicolonPos != std::string::npos) {
        contentType = contentType.substr(0, semicolonPos);
    }
    if (contentType == "multipart/form-data") {
        parser = new HTTPMultipartBodyParser(req);
    }
    else {
        Serial.printf("Unknown POST Content-Type: %s\n", contentType.c_str());
        return;
    }
    std::string filename;
    while (parser->nextField()) {
        std::string name = parser->getFieldName();
        Serial.printf("Parsing field %s\n", name.c_str());
        char buf[512];
        if (name == "filename") {

            size_t readLength = parser->read((byte*)buf, 512);
            filename = std::string("/") + std::string(buf, readLength);
        }
        else if (name == "data") {
            filename = parser->getFieldFilename();
            //size_t readLength = parser->read((byte*)buf, 512);
            //std::string filePath = std::string(buf, readLength);
            //check if it comes from 
            int startIdx = filename.find_first_of("?");
            if (startIdx < 0) //no filename yet
            {
                //    res->printf("File path not valid\n");
                //    break;
                //} else
                //if (startIdx == 0) //no filename yet
                //{
                res->printf("POST.. no prefix\n");
                //break;
            }
            else {
                startIdx++;
                filename = filename.substr(startIdx);
            }

            // Serial.printf("Uploading %s\n", filename.c_str());
             //Serial.printf("Parsing substring from idx %d\n", ++startIdx);

            if (filename == "") {
                res->println("<p>Error: form contained content before filename.</p>");
                break;
            }
            else
            {
                Serial.printf("Writing %u bytes to file [%s]..", req->getContentLength(), filename.c_str());
                size_t fieldLength = 0;
                //if(!SPIFFS.exists(filename.c_str())) break;
                File file = SPIFFS.open(filename.c_str(), "w");
                savedFile = true;
                while (!parser->endOfField()) {
                    byte buf[512];
                    size_t readLength = parser->read(buf, 512);
                    file.write(buf, readLength);
                    fieldLength += readLength;
                }
                file.close();
                Serial.println("Done.");
                res->printf("<p>Saved %d bytes to %s</p>", int(fieldLength), filename.c_str());
            }
        }
        else if (name == "path"){
            string fileName = "";
            byte buf[512];
            //adding
            while (!parser->endOfField()) {                
                memset(buf,0,sizeof(buf));
                size_t readLength = parser->read(buf, 512);
                filename.append((const char *)buf);
            }
            Serial.printf("File to create: %s\n", filename.c_str());
            SPIFFS.open(filename.c_str(),"w");
            
        }
        else {
            res->printf("<p>Unexpected field %s</p>", name.c_str());
        }
    }
    if (!savedFile) {
        res->println("<p>No file to save...</p>");
    }
    res->println("</body></html>");
    // Serial.printf("Uploading file %s completed successfully!\n", name.c_str());

}
// This is the internal page. It will greet the user with
// a personalized message and - if the user is in the ADMIN group -
// provide a link to the admin interface.
void esp32_router::handleInternalPage(HTTPRequest* req, HTTPResponse* res) {
    // Header
    res->setStatusCode(200);
    res->setStatusText("OK");
    res->setHeader("Content-Type", "text/html; charset=utf8");

    // Write page
    res->println("<!DOCTYPE html>");
    res->println("<html>");
    res->println("<head>");
    res->println("<title>Internal Area</title>");
    res->println("<script src = \"/JS/auth.js\"></script>");
    res->println("</head>");
    res->println("<body>");

    // Personalized greeting
    res->printf("<h1>Hello %s !</h1>", req->getHeader(HEADER_USERNAME).c_str());
    // We can safely use the header value, this area is only accessible if it's
    // set (the middleware takes care of this)
    
    res->println("<p>Welcome to the internal area. Congratulations on successfully entering your password!</p>");

    // The "admin area" will only be shown if the correct group has been assigned in the authenticationMiddleware
    if (req->getHeader(HEADER_GROUP) == "ADMIN") {
        res->println("<div style=\"border:1px solid red;margin: 20px auto;padding:10px;background:#ff8080\">");
        res->println("<h2>You are an administrator</h2>");
        res->println("<p>You are allowed to access the admin page:</p>");
        res->println("<p><a href=\"#\" onclick=\"link('/internal/admin')\">Go to secret admin page</a></p>");
        res->println("</div>");
    }

    // Link to the root page
    res->println("<p><a href=\"/\">Go back home</a></p>");
    res->println("</body>");
    res->println("</html>");
}

void esp32_router::handleAdminPage(HTTPRequest* req, HTTPResponse* res) {
    // Headers
    res->setHeader("Content-Type", "text/html; charset=utf8");

    std::string header = "<!DOCTYPE html><html><head><title>Secret Admin Page</title><script src = \"/JS/auth.js\"></script></head><body><h1>Secret Admin Page</h1>";
    std::string footer = "</body></html>";

    // Checking permissions can not only be done centrally in the middleware function but also in the actual request handler.
    // This would be handy if you provide an API with lists of resources, but access rights are defined object-based.
    if (req->getHeader(HEADER_GROUP) == "ADMIN") {
        res->setStatusCode(200);
        res->setStatusText("OK");
        res->printStd(header);
        res->println("<div style=\"border:1px solid red;margin: 20px auto;padding:10px;background:#ff8080\">");
        res->println("<h1>Congratulations</h1>");
        res->println("<p>You found the secret administrator page!</p>");
        res->println("<p><a href=\"#\" onclick=\"link('/internal')\">Go back</a></p>");
        res->println("</div>");
    }
    else {
        res->printStd(header);
        res->setStatusCode(403);
        res->setStatusText("Unauthorized");
        res->println("<p><strong>403 Unauthorized</strong> You have no power here!</p>");
    }

    res->printStd(footer);
}

// Just a simple page for demonstration, very similar to the root page.
void esp32_router::handlePublicPage(HTTPRequest* req, HTTPResponse* res) {   
   /* res->setHeader("Content-Type", "text/html");
    res->println("<!DOCTYPE html>");
    res->println("<html>");
    res->println("<head><title>Hello World!</title></head>");
    res->println("<body>");
    res->println("<h1>Hello World!</h1>");
    res->print("<p>Your server is running for ");
    res->print((int)(millis() / 1000), DEC);
    res->println(" seconds.</p>");
    res->println("<p><a href=\"/\">Go back</a></p>");
    res->println("</body>");
    res->println("</html>");*/
    std::string content = "<H1>Public Page</h1>\n"
        "<p>Your server is running for\n";
    content.append(String((int)millis() / 100).c_str());
    content.append(" seconds.</p>");
    handleRoot(req, res, &content);
}

void esp32_router::handleRoot(HTTPRequest* req, HTTPResponse* res,std::string* content) {
   
    
    res->setHeader("Content-Type", "text/html");

    esp32_controller_route route;
    if (GetControllerRoute(req,route)) {
        //route is under controller
        
        auto controllerObj = /*controllerFactory->hasInstance(route.controller) ? */
            controllerFactory->createInstance(route);// : NULL;
        
        //esp32_template controllerTemplate = esp32_template();
        if (controllerObj == NULL) {
            handle404(req, res);
            return;
        }
       
        controllerObj->Action(req, res);

        if(strcmp(req->getMethod().c_str(),"GET") != 0)
            return;

        if(!controllerObj->HasAction(route.action.c_str())) return; //custom action or 404

        //serve get request using template engine
        String path = SITE_ROOT;
        path += "/T/_template.html";
        File f = SPIFFS.open(path);
        res->setStatusCode(200);
        res->setStatusText("OK");
        res->setHeader("Content-Type", "text/html");
        Serial.printf("[esp32_router::handleRoot] Opening file %s\n", f.path());
        while (f.available()) {
            String line = f.readStringUntil('\n');
            //if we already processed a section, skip trying to reprocess
            int titleidx = 0,
                headidx = 0,
                menuidx = 0,
                headeridx = 0,
                contentidx = 0,
                footeridx = 0;

            titleidx = handlePagePart_Title(req, res, line, controllerObj->title);
            if (titleidx > 0) {
                continue;
            }
            headidx = handlePagePart_Head(req, res, line, controllerObj->head);
            if (headidx > 0) {
                continue;
            }
            menuidx = handlePagePart_Menu(req, res, line, controllerObj->menu);
            if (menuidx > 0) {
               continue;
            }
            headeridx = handlePagePart_Header(req, res, line,"");//TODO: oops .. missed that one
            if (headeridx > 0) {
                continue;
            }
            //serve content from stream or controller object
            content != nullptr && content->length() > 0 ?
                contentidx = handlePagePart_Content(req, res, line, *content):
                handlePagePart_Content(req, res, line, controllerObj);
            if (contentidx > 0) {
               continue;
            }
            footeridx = handlePagePart_Footer(req, res, line,controllerObj->footer);
            if (footeridx > 0) {
                continue;
            }


            bool anyReplaced = titleidx >= 0 || headidx >= 0 || headeridx >= 0 || menuidx >= 0 || contentidx >= 0 || footeridx >= 0;
            if (!anyReplaced) {
               /* Serial.printf("[CONTENT WRITER] %s\n", line.c_str());*/
                res->println(line);
            }

        }
        f.close();
        Serial.printf("[ESP ROUTER]Serving page from template %s\n", path.c_str());
        //writeFileToResponse(path.c_str(), res);
    }
    else
    {
        Serial.printf("[ESP ROUTER]Serving page from file %s\n", req->getRequestString().c_str());
        writeFileToResponse(req, res);
    }

    

    

    //TODO: fill menu and file from spiffs
}

//void handleEditPage(HTTPRequest* req, HTTPResponse* res) {
//    String path = SITE_ROOT;
//    path += "/edit.htm.gz";
//    File f = SPIFFS.open(path);
//
//    if (f.size() <= 0) return;
//    Serial.print("Providing Edit.htm.gz file "); Serial.print(f.size()); Serial.println(" bytes.");
//    res->setHeader("Content-Encoding", "gzip");
//    char buff[256];
//    while (true) {
//        
//        uint16_t bytestoRead = f.available() < 256 ? f.available() : 256;
//        if (bytestoRead == 0) break;
//        f.readBytes(buff, bytestoRead);
//        res->write((uint8_t*)buff, bytestoRead);
//        Serial.printf("Wrote %u bytes from %s\n", bytestoRead, f.name());
//    }
//
//}
void esp32_router::writeFileToResponse(HTTPRequest* req, HTTPResponse* res){
    //Check if we zip
    bool useGZ = false;
    bool download = false;
    //check if gz version exists
    String fileName = String(req->getRequestString().c_str());
    if (!fileName.startsWith(SITE_ROOT))
        fileName = SITE_ROOT + fileName;

    Serial.printf("[3.1] Loading file %s.\n", fileName.c_str());
    //url decode
    fileName = urlDecode(fileName.c_str()).c_str();
    //if part of parameter
    int startIdx = fileName.indexOf("?");
    if (startIdx > 0) {
        startIdx++;
        fileName = fileName.substring(startIdx);
    }
    //check if we have an end
    int endIdx = fileName.indexOf("?");
    if (endIdx > 0) {
        if (fileName.substring(endIdx + 1).compareTo("download=true") == 0)
            download = true;
        else
            //Serial.printf("Found second ? but expected download=true not found. [%s]\n", fileName.substring(endIdx + 1).c_str());
        fileName = fileName.substring(0, endIdx);
    }

    String gzName = fileName + ".gz";
    if (SPIFFS.exists(gzName))
        useGZ = true;
    String ext = fileName.substring(fileName.lastIndexOf('.') + 1).c_str();

    File f;
    Serial.printf("[3.2] Loading file %s.\n", (useGZ ? gzName : fileName).c_str());
    if (!SPIFFS.exists(useGZ ? gzName : fileName)) {
        //Serial.printf("File not found: %s\n", useGZ ? gzName.c_str() : fileName.c_str());
        string fileName = SITE_ROOT;
        fileName +=
            ext == "js" ? "/JS/" :
            ext == "css" ? "/CSS/" : "/";
        fileName += req->getRequestString().substr(fileName.find_last_of('/') + 1).c_str();
       //Serial.printf("[HTTP]1. Opening file %s\n", fileName.c_str());
        f = SPIFFS.open(fileName.c_str());
        if (f.size() < 0)
        {
            //finnaly check if its gz file
            fileName += ".gz";
            //Serial.printf("[1.1 - GZ loosely linked] Opening file %s\n", fileName.c_str());
            f = SPIFFS.open(fileName.c_str());
            if (f.size() <= 0)
                return;
            else useGZ = true;
        }
    }
    else {
        Serial.printf("Found file %s in %s format with extention %s\n", useGZ ? gzName.c_str() : fileName.c_str(), useGZ ? "GZ" : "REG", ext);
        f = SPIFFS.open(useGZ ? gzName : fileName);
    }
    // if (f.size() <= 0) {
    //     f.close();
    //     //try corrected path

    //     String fileName = SITE_ROOT;
    //     fileName +=
    //         ext == "js" ? "/JS/" :
    //         ext == "css" ? "/CSS/" : "/";
    //     fileName += req->getRequestString().substr(req->getRequestString().find_last_of('/') + 1).c_str();
    //     Serial.printf("[HTTP]2. Opening file %s\n", fileName.c_str());
    //     f = SPIFFS.open(fileName);
    //     if (f.size() <= 0)
    //     {
    //         f.close();
    //         //finnaly check if its gz file
    //         fileName += ".gz";
    //         Serial.printf("[2.1 - GZ loosely linked] Opening file %s\n", fileName.c_str());
    //         f = SPIFFS.open(fileName);
    //         if (f.size() <= 0){
    //             Serial.printf("[2.2 - GZ Failed to load. Quitting]");
    //             return;
    //         }
    //         else useGZ = true;
    //     }
    // }
    Serial.printf("Providing %s file ", f.name());  Serial.print(f.size()); Serial.println(" bytes.");
    if (useGZ) res->setHeader("Content-Encoding", "gzip");
    res->setHeader("Cache-Control", strstr(f.name(),"list?") != nullptr || String(req->getHeader("Refer").c_str()).endsWith("edit.html") ? "no-store" : "private, max-age=604800");
    if (download) {
        char dispStr[128];
        sprintf(dispStr, " attachment; filename = \"%s\"", fileName);
        res->setHeader("Content - Disposition", dispStr);
        ext = "application/octet-stream";
    }
    else {
        if (ext == "htm") ext = "html"; //workaround for encoding
        else if (ext == "js") ext = "javascript"; //workaround for encoding
        ext = "text/" + ext;
    }
    res->setHeader("Content-Type", ext.c_str());
    char buff[256];
    while (true) {

        uint16_t bytestoRead = f.available() < 256 ? f.available() : 256;
        if (bytestoRead == 0) break;
        f.readBytes(buff, bytestoRead);
        res->write((uint8_t*)buff, bytestoRead);
        //Serial.printf("Wrote %u bytes to %s\n", bytestoRead, f.name());
    }
}
void esp32_router::handleFile(HTTPRequest* req, HTTPResponse* res) {
    writeFileToResponse(req, res);        
}


void esp32_router::handle404(HTTPRequest* req, HTTPResponse* res) {
    req->discardRequestBody();
    res->setStatusCode(404);
    res->setStatusText("Not Found");
    res->setHeader("Content-Type", "text/html");
    res->println("<!DOCTYPE html>");
    res->println("<html>");
    res->println("<head><title>Not Found</title></head>");
    res->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
    res->println("</html>");
}


#endif

