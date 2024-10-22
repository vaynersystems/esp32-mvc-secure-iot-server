#ifndef _ESP32_ROUTER_CPP
#define _ESP32_ROUTER_CPP
#include "esp32_router.h"
#include "System/CORE/esp32_server.hpp"

extern esp32_server server;

// void esp32_router::InitConfigurableRouting()
// {
// }

void esp32_router::RegisterHandler(ResourceNode *resourceNode)
{
    RegisterHandler(resourceNode->_path.c_str(), resourceNode->getMethod().c_str(), resourceNode->_callback);
}
void esp32_router::RegisterWebsocket(WebsocketNode *resourceNode)
{
    server.registerNode(resourceNode);
}

void esp32_router::RegisterHandler(String nodeMapPath, HTTPMETHOD method, HTTPSCallbackFunction *handler)
{
    String methodName;
    switch (method)
    {
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
    RegisterHandler(nodeMapPath, methodName, handler);
}

void esp32_router::RegisterHandler(String nodeMapPath, String method, HTTPSCallbackFunction *handler)
{
    ResourceNode *node = new ResourceNode(nodeMapPath.c_str(), method.c_str(), handler);
    server.registerNode(node);
}

void esp32_router::handleCORS(HTTPRequest *req, HTTPResponse *res)
{
    // Serial.println(">> CORS here...");
    res->setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    res->setHeader("Access-Control-Allow-Origin", "*");
    res->setHeader("Access-Control-Allow-Headers", "*");
}

void esp32_router::handleFileList(HTTPRequest *req, HTTPResponse *res)
{
    string dir, path, filter, driveStr;
    req->getParams()->getQueryParameter("dir", dir);
    req->getParams()->getQueryParameter("path", path);
    req->getParams()->getQueryParameter("filter", filter);
    req->getParams()->getQueryParameter("drive", driveStr);
    if(!is_number(driveStr)){
        driveStr="0"; //default to drive 0
    }
    auto drive = filesystem.getDisk(atoi(driveStr.c_str()));

    res->setHeader("Content-Type", "application/json");
    if (path.length() > 0)
    {
        // search path
        if (path.length() > 1 && path[path.length() - 1] == '/')
            path = path.substr(0, path.length() - 1);

        drive->list(path.c_str(), res, JSON,filter.c_str(), 200, 5);       
        
    }
    else
    {
        // print dir recursivly
        if (dir.length() <= 0)
        {
            dir = "/";
            #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
            Serial.printf("Empty path, starting from %s\n", dir.c_str());
            #endif
        }

        drive->list(dir.c_str(), res, JSON, filter.c_str(), 200, 5);    
    }
}

int esp32_router::handlePagePart_Title(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_TITLE, (req->getHeader(HEADER_AUTH).length() > 0) ? "/T/title_int.html" : "/T/title_pub.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_TITLE, content);
}

int esp32_router::handlePagePart_Head(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_HEAD, "/T/head.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_HEAD, content);
}

int esp32_router::handlePagePart_Header(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_HEADER, (req->getHeader(HEADER_GROUP).length() > 0) ? "/T/header_int.html" : "/T/header_pub.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_HEADER, content);
}

int esp32_router::handlePagePart_Menu(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
    {
        if(req->getHeader(HEADER_GROUP).length() <= 0){
            return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_MENU, "/T/menu_pub.html");
        }
        //Serial.printf("Authenticated user == %s.\n", req->getHeader(HEADER_GROUP).c_str());
        int idx = line.indexOf(HTML_REF_CONST_MENU);
        if (idx < 0)
            return idx;
        res->print(line.substring(0, idx));

        auto headerModule =                                        /*controllerFactory->hasInstance(route.controller) ? */
            controllerFactory->createInstance("_menu_int", "index"); // : NULL;

        if (headerModule != NULL)
        {
            // module found
            headerModule->Action(req, res); // execute module action
            handlePagePart_Content(req, res, HTML_REF_CONST_CONTENT, headerModule); //render menu template content
            delete headerModule;
            res->println(line.substring(idx + sizeof(HTML_REF_CONST_MENU) - 1)); //finish the rest of the global template line
            return idx;
        }
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_MENU, "/T/menu_pub.html");
    }

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_MENU, content);

    // if (content.length() <= 0)
    //     return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_MENU, (req->getHeader(HEADER_GROUP).length() > 0) ? "/T/menu_int.html" : "/T/menu_pub.html");

    // return handlePagePart_FromString(req, res, line, HTML_REF_CONST_MENU, content);
}

int esp32_router::handlePagePart_Content(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
    {
        int idx = line.indexOf(HTML_REF_CONST_CONTENT);
        if (idx < 0)
            return idx;
        res->print(line.substring(0, idx));
        handleFile(req, res);

        res->println(line.substring(idx + sizeof(HTML_REF_CONST_CONTENT) - 1));

        return idx;
    }

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_CONTENT, content);
}

int esp32_router::handlePagePart_Content(HTTPRequest *req, HTTPResponse *res, String line, esp32_base_controller *controller)
{
    int idx = line.indexOf(HTML_REF_CONST_CONTENT);
    if (idx < 0)
        return idx;
    res->print(line.substring(0, idx));
    if (!controller->controllerTemplate.RenderTemplate(req, res))
        handle404(req, res); // if not able to render template, handle with 404
    controller->controllerTemplate.ClearVariables();
    res->println(line.substring(idx + sizeof(HTML_REF_CONST_CONTENT) - 1));
    return idx;
}


int esp32_router::handlePagePart_Footer(HTTPRequest *req, HTTPResponse *res, String line, string content = "")
{
    if (content.length() <= 0)
    {
        int idx = line.indexOf(HTML_REF_CONST_FOOTER);
        if (idx < 0)
            return idx;
        res->print(line.substring(0, idx));

        auto footerModule =                                        /*controllerFactory->hasInstance(route.controller) ? */
            controllerFactory->createInstance("_footer", "index"); // : NULL;

        if (footerModule != NULL)
        {
            // module found
            footerModule->Action(req, res); // execute module action
            handlePagePart_Content(req, res, HTML_REF_CONST_CONTENT, footerModule);
            // //render module output
            // if (!footerModule->controllerTemplate.RenderTemplate(req, res)){
            //     HTTPS_LOGE("Failure to render footer template");
            // }

            delete footerModule;
            res->println(line.substring(idx + sizeof(HTML_REF_CONST_FOOTER) - 1));
            return idx;
        }
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_FOOTER, "/T/V/_footer.html");
    }

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_FOOTER, content);
}

int esp32_router::handlePagePart_FromFile(HTTPRequest *req, HTTPResponse *res, String line, const char *searchString, String fileName)
{
    auto drive = filesystem.getDisk(SYSTEM_DRIVE);
    int idx = line.indexOf(searchString);
    if (idx >= 0)
    {
        if (!fileName.startsWith(PATH_SITE_ROOT))
            fileName = PATH_SITE_ROOT + fileName;

        // Serial.printf("\t[PagePart Parser]. Found %s in %s. Filling from %s. \n", searchString, line.c_str(), fileName.c_str());
        res->print(line.substring(0, idx).c_str());

        if (drive->exists(fileName.c_str()))
        {
            File fPagePart = drive->open(fileName.c_str());

            while (fPagePart.available())
            {
                String docLine = fPagePart.readStringUntil('\n');
                res->print(docLine.c_str());
                if (fPagePart.available())
                    res->print('\n'); // write new line if not last line
            }
            fPagePart.close();
        }
        else
        {
            #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
            Serial.printf("File %s not found.\n", fileName.c_str());
            #endif
            res->printf("Page Part %s file %s NOT FOUND!", searchString, fileName.c_str());
        }
        res->println(line.substring(idx + strlen(searchString)).c_str());
    }

    return idx; // > 0 ? idx : 0;
}
int esp32_router::handlePagePart_FromString(HTTPRequest *req, HTTPResponse *res, String line, const char *searchString, string content)
{

    int idx = line.indexOf(searchString);
    if (idx >= 0)
    {
        res->print(line.substring(0, idx).c_str());
        res->print(content.c_str());
        res->println(line.substring(idx + strlen(searchString)).c_str());
    }

    return idx; // > 0 ? idx : 0;
}
// Get the route information for a controler/action syntax. returns true if controller found, false otherwise
bool esp32_router::GetControllerRoute(HTTPRequest *request, esp32_controller_route &routeObj)
{
    string controller(""), action("");
    string pathString(""), queryString("");

    pathString = request->getRequestString();
    // Decode path:
    // 1. Check for query string, if present, parse it out
    // 2. get controller.
    // 3. check if action is specified, if so parse it. otherwise default to html method as action

    int queryStart = pathString.find_first_of('?');
    if (queryStart > 0)
    {
        queryString = pathString.substr(queryStart + 1);
        pathString = pathString.substr(0, queryStart);
        // Serial.printf("Query string detected with %s path and %s query\n", pathString.c_str(), queryString.c_str());
        // reqString.erase(queryStart);
    }
    if (pathString.find_first_of('/') == 0) // remove leading slash
        pathString.erase(0, 1);

    // default controller if none is specified
    controller = "error";
    if (strstr(pathString.c_str(), "index.html") != nullptr || pathString.length() == 0)
        controller = "esp32_home";
    else
        controller = pathString;

    int reqIdxSlash = pathString.find_first_of('/');
    bool actionFound = false;
    if (reqIdxSlash > 0) // explicit action defined
    {
        controller = pathString.substr(0, reqIdxSlash);
        pathString.erase(0, reqIdxSlash + 1);
        action = pathString;
        actionFound = true;
    }
    else // default action
        action = "index";

    // if(iequals(request->getMethod().c_str(),"GET",3)){
    //     action = request->getMethod();
    // }else {
    reqIdxSlash = pathString.find_first_of('/');
    if (reqIdxSlash > 0)
    {
        action = pathString.substr(0, reqIdxSlash);
        pathString.erase(0, reqIdxSlash + 1);
    }
    else
    {
        if (actionFound)
            action = pathString;
        else if (!iequals(request->getMethod().c_str(), "GET", 3))
            action = request->getMethod();
        if (pathString[0] == '/')
            pathString.erase(0);
    }
    //}
    // Serial.printf("Parsed url. Controller=%s Action=%s Remainder=%s Query=%s\n", controller.c_str(), action.c_str(), pathString.c_str(), queryString.c_str());

    routeObj.action = action;
    routeObj.controller = controller;
    routeObj.params = urlDecode(queryString);
    // return IsValidRoute(routeObj);
    return controllerFactory->hasInstance(controller.c_str());
}

bool esp32_router::IsValidRoute(esp32_controller_route &route)
{
    int numOfControllers = BaseControllerFactory::getInstanceCount();
    // Serial.printf("Found %i controllers\n", numOfControllers);
    for (int i = 0; i < numOfControllers; i++)
    {
        auto controller = BaseControllerFactory::getInstanceAt(i);
        // if(controller.first[0] == '_') continue;

        vector<string> actions = {};
        controller.second()->GetActions(&actions);
        // Serial.printf("Controller %d of %d: %s with %i actions\n",i, numOfControllers,controller.first.c_str(), actions.size());
        for (const string &action : actions)
        {
            if (route.controller.length() == controller.first.length() && route.action.length() == action.length())
            {
                if (iequals(route.controller.c_str(), controller.first.c_str(), route.controller.length()) && iequals(route.action.c_str(), action.c_str(), action.length()))
                {
                    // Serial.printf("Matched route controller %s action %s against cataloged controller %s action %s\n", route.controller.c_str(), route.action.c_str(),controller.first.c_str(), action.c_str());
                    // case correct action
                    route.action = action;
                    return true;
                }
            }
        }
    }
    return false;
}
void esp32_router::handleFileUpload(HTTPRequest *req, HTTPResponse *res){
    handleFileUpload(req, res, NULL);
}
void esp32_router::handleFileUpload(HTTPRequest *req, HTTPResponse *res, const char * overwriteFilePath)
{
    string drive="";
    bool authorized = strcmp(req->getHeader(HEADER_GROUP).c_str(), "ADMIN") == 0;
    if(!authorized){
        handle401(req,res);
        return;
    }
    if (req->getMethod() == "DELETE")
    {

        HTTPURLEncodedBodyParser parser(req);
        string filename, filePath;
        bool savedFile = false;
        while (parser.nextField())
        {
            string name = parser.getFieldName();
            // Serial.printf("Parsing field %s\n", name.c_str());
            if(
                (name.substr(0, strlen("------WebKitFormBoundary")).compare("------WebKitFormBoundary") == 0) || //seems like different browsers like to do it differnetly
                (name.substr(0, strlen("------------------------")).compare("------------------------") == 0)
            )
            {
                byte buf[64];
                // adding
                while (!parser.endOfField())
                {
                    memset(buf, 0, sizeof(buf));
                    size_t readLength = parser.read(buf, sizeof(buf));
                    filename.append((const char *)buf);
                }
                filePath = filename;
                filePath = filePath.erase(0, filePath.find_first_of("/"));
                filePath = filePath.erase(filePath.find_first_of(13));
                #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
                Serial.printf("Received request to delete %s\n", filePath.c_str());
                #endif
                // filename = filePath.substr(filePath.find_first_of('?') + 1);

                bool deleted = esp32_fileio::DeleteFile(filePath.c_str());
                if (!deleted)
                {
                    res->printf("File [%s] not found.", filePath.c_str());
                }
                else
                {
                    res->printf("<p>Deleted %s</p>", filePath.c_str());
                    savedFile = true;
                }
            }
            else
            {
                #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
                Serial.printf("Expected string %s. Instead found %s\n", "------------------------", name.substr(0, strlen("------------------------")).c_str());
                #endif
            }
            /* else {
                 res->printf("<p>Unexpected field %s</p>", name.c_str());
             }*/
        }
        if (!savedFile)
        {
            res->println("<p>No file to delete...</p>");
        }
        res->println("</body></html>");
        return;
    }
    else if (req->getMethod() == "GET")
    {
    }
    else if (req->getMethod() == "POST" || req->getMethod() == "PUT")
    {

        res->println("<html><head><title>File Edited</title><head><body><h1>File Edited</h1>");
        HTTPMultipartBodyParser *parser;
        string contentType = req->getHeader("Content-Type");
        size_t semicolonPos = contentType.find(";");
        bool savedFile = false;
        if (semicolonPos != string::npos)
        {
            contentType = contentType.substr(0, semicolonPos);
        }
        if (contentType == "multipart/form-data")
        {
            parser = new HTTPMultipartBodyParser(req);
        }
        else
        {
            #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
            Serial.printf("Unknown POST Content-Type: %s\n", contentType.c_str());
            #endif
            return;
        }
        string filename, path;
        while (parser->nextField())
        {
            string name = parser->getFieldName();
            string mimeType = parser->getFieldMimeType();
            #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
                Serial.printf("Parsing field %s\n", name.c_str());
            #endif
            char buf[512];
            if (name == "filename")
            {

                size_t readLength = parser->read((byte *)buf, 512);
                filename = string("/") + string(buf, readLength);
                
                if(drive.length() > 0) break;
                
            }
            else if (name == "data" || name == "file")
            {
                filename = parser->getFieldFilename();
                int startIdx = filename.find_first_of("?");
                if (startIdx < 0) // no filename yet
                {
                    res->printf("POST.. no prefix\n");
                }
                else
                {
                    startIdx++;
                    filename = filename.substr(startIdx);
                }

                if (filename == "")
                {
                    res->println("<p>Error: form contained content before filename.</p>");
                    break;
                }
                else
                {
                    //if want to overwrite filename, do it here
                    if(overwriteFilePath != NULL)
                        filename = overwriteFilePath;                    
                }
                break;
            }else if(name == "drive"){
                while (!parser->endOfField())
                {
                    memset(buf, 0, sizeof(buf));
                    size_t readLength = parser->read((byte*)buf, 512);
                    drive.append((const char *)buf);
                }              
                //if(filename.length() > 0) break;
                
            }
            else if (name == "path")
            {                
                // adding
                while (!parser->endOfField())
                {
                    memset(buf, 0, sizeof(buf));
                    size_t readLength = parser->read((byte*)buf, 512);
                    path.append((const char *)buf);
                }  
                //if(drive.length() > 0) break;               
            }
            else
            {
                res->printf("<p>Unexpected field %s</p>", name.c_str());
            }
        }
        //prefix drive if speificed
        if(drive.length() > 0){
            
            auto fs = filesystem.getDisk(atoi(drive.c_str()));
            if(path.find(fs->label()) <= 0) //if drive not passed in filename, prefix it
                path = string_format("/%s%s",fs->label(),path.c_str());   
            #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0           
            Serial.printf("Drive parameter: %s, Path: %s, Filename: %s\n", drive.c_str(), path.c_str(), filename.c_str());
            #endif
        }

        unsigned long startWrite = millis();
        if(req->getMethod() == "PUT")
            savedFile = esp32_fileio::CreateFile(path.c_str());

        if(req->getMethod() == "POST")
        {
            if(path.length() > 0)
                filename = path + "/" + filename;
            if(filename[filename.length() - 1] == '/'){
                #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
                Serial.printf("Filename %s seems to be a directory. Qutting upload");
                #endif
                res->setStatusCode(500);
            } else{                
                size_t bytes = esp32_fileio::UpdateFile(filename.c_str(), parser,true);
                #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
                Serial.printf("Writing %u bytes to file [%s].. ", bytes, filename.c_str());
                #endif
                savedFile = bytes > 0;
                res->printf("<p>Saved %d bytes to %s</p>", bytes, filename.c_str());
            }
            
        }
        #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
        Serial.printf(" done in %u ms\n", millis() - startWrite);
        #endif

        if (!savedFile)
        {
            res->println("<p>No file to save...</p>");
        }
        res->println("</body></html>");

        
        // Serial.printf("Uploading file %s completed successfully!\n", name.c_str());
    }
}

void esp32_router::handleRoot(HTTPRequest *req, HTTPResponse *res)
{

    res->setHeader("Content-Type", "text/html");

    esp32_controller_route route;
    if (GetControllerRoute(req, route))
    {
        handleControllerRequest(req, res, route);        
    }
    else
    {
        #if defined(DEBUG_FILESYSTEM) && DEBUG_FILESYSTEM > 0
        Serial.printf("[ESP ROUTER]Serving page from file %s\n", req->getRequestString().c_str());
        #endif
        handleFile(req, res);
    }
}

void esp32_router::handleFile(HTTPRequest *req, HTTPResponse *res)
{
    bool isAdminUser = req->getHeader(HEADER_GROUP) == "ADMIN";    
    //hydrate to check if path is internal, user authorized, and file exists.
    //Serial.printf("[ESP32_ROUTER] handling file %s\n", req->getRequestString().c_str());
    auto routeInfo = esp32_route_file_info<esp32_file_info_extended>(req);
    
    if(routeInfo.isInternal && !isAdminUser){
        res->setStatusCode(401);
        return;
    }   

    if(!routeInfo.exists())
    {
        handle404(req, res);
        return;
    }   
    
    esp32_fileio::writeFileToResponse(urlDecode(req->getRequestString()).c_str(), res);
}


#if ENABLE_EDITOR   	
void esp32_router::handleEditor(HTTPRequest *req, HTTPResponse *res)
{
    
    bool authorized = strcmp(req->getHeader(HEADER_GROUP).c_str(), "ADMIN") == 0;
    if(!authorized){
        handle401(req,res);
        return;
    }
    auto routeInfo = esp32_route_file_info<esp32_file_info_extended>("/W/edit.html");
    esp32_fileio::writeFileToResponse(routeInfo, res);

}
#endif
void esp32_router::handle404(HTTPRequest *req, HTTPResponse *res)
{
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

void esp32_router::handle401(HTTPRequest *req, HTTPResponse *res)
{
    req->discardRequestBody();
    res->setStatusCode(404);
    res->setStatusText("Unauthorized");
    res->setHeader("Content-Type", "text/html");
    res->println("<!DOCTYPE html>");
    res->println("<html>");
    res->println("<head><title>Unauthorized</title></head>");
    res->println("<body><h1>401 Unauthorized</h1><p>The requested was not authorized for this resource.</p></body>");
    res->println("</html>");
}

void esp32_router::handleControllerRequest(HTTPRequest *req, HTTPResponse *res, esp32_controller_route route)
{
    // route is under controller
    auto controllerObj = controllerFactory->createInstance(route);
    if (controllerObj == NULL)
    {
        res->println("Controller not found");
        handle404(req, res);
        return;
    }

    if (!controllerObj->HasAction(route.action.c_str()))
    {
        esp32_router::handle404(req, res);
    }
    else if(!controllerObj->Authorized(req)){
        handle401(req,res);
    }
    else
    {
        controllerObj->Action(req, res);
        controllerObj->controllerTemplate.ClearVariables();

        //Serial.printf("[ESP ROUTER]Serving page from template %s\n", route.controller.c_str());
    }
    delete controllerObj;
}


string esp32_router::handleServiceRequest(esp32_service_route route){
    auto service = serviceFactory->createInstance(route);
    if (service == NULL)
    {
        #if defined(DEBUG_SOCKET) && DEBUG_SOCKET > 0
        Serial.printf("Service %s not found\n", route.service.c_str());
        #endif
        return "error";
    }

    string returnValue = "";
    returnValue = service->Execute();
    delete service;
    return returnValue;
}

#endif
