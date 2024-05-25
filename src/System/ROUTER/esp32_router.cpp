#ifndef _ESP32_ROUTER_CPP
#define _ESP32_ROUTER_CPP
#include "esp32_router.h"
#include "System/CORE/esp32_server.h"

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

void esp32_router::RegisterHandlers(fs::FS &fs, const char *dirname, uint8_t levels)
{
    vector<esp32_route_file_info> files = vector<esp32_route_file_info>();
    esp32_fileio::getFiles(fs, files, dirname);
    //all files will have preceeding dirname of site root. remove it in registering
    for(int idx = 0; idx < files.size();idx++){
        RegisterHandler(files[idx].filePath.substr(2).c_str(),HTTPMETHOD_GET, &handleFile);
        if(files[idx].isGZ)
        {
            string fileGz = files[idx].fileName.substr(2) + ".gz";
            RegisterHandler(fileGz.c_str(),HTTPMETHOD_GET, &handleFile);
        }
    }

    // File root = fs.open(dirname);
    // if (!root || !root.isDirectory())
    // {
    //     return;
    // }

    // File file = root.openNextFile();
    // while (file)
    // {
    //     if (file.isDirectory())
    //     {
    //         if (levels)
    //         { // print out directory contents to serial
    //             esp32_fileio::listDir(fs, &Serial, file.name(), levels - 1);
    //         }
    //     }
    //     else
    //     { // Register file handler as filename

    //         String mappingPath = String(file.name());
    //         mappingPath.remove(0, sizeof(PATH_SITE_ROOT) - 1);

    //         RegisterHandler(mappingPath.c_str(), HTTPMETHOD_GET, &handleFile);

    //         int startIdx = mappingPath.lastIndexOf('/');
    //         if (startIdx > 0)
    //         {
    //             String filePath = mappingPath.substring(startIdx);
    //             if (filePath.length() > 0)
    //                 RegisterHandler(filePath.c_str(), HTTPMETHOD_GET, &handleFile);
    //         }

    //         // if gz file, register handler without gz
    //         if (String(file.name()).endsWith(".gz"))
    //         {
    //             String nonGZName = String(file.name());
    //             nonGZName.remove(nonGZName.length() - 3);

    //             RegisterHandler(nonGZName.c_str(), HTTPMETHOD_GET, &handleFile);

    //             nonGZName.remove(0, sizeof(PATH_SITE_ROOT) - 1);
    //             // Serial.printf(" Resistering GZ Handler[%s , %s]\n", file.name(), nonGZName.c_str());
    //             RegisterHandler(nonGZName.c_str(), HTTPMETHOD_GET, &handleFile);
    //         }
    //     }
    //     file = root.openNextFile();
    // }
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
    string dir, path, filter;
    req->getParams()->getQueryParameter("dir", dir);
    req->getParams()->getQueryParameter("path", path);
    req->getParams()->getQueryParameter("filter", filter);

    res->setHeader("Content-Type", "application/json");
    if (path.length() > 0)
    {
        // search path
        if (path.length() > 1 && path[path.length() - 1] == '/')
            path = path.substr(0, path.length() - 1);

        list<SPIFFS_FileInfo> files = list<SPIFFS_FileInfo>();

        // Serial.printf("Searching for files only in path %s\n", path.c_str());
        esp32_fileio::buildOrderedFileList(SPIFFS, path.c_str(), filter.c_str(), 3, files);
        esp32_fileio::printFileSearchOrdered(res, &files, filter);
    }
    else
    {
        // print dir recursivly
        if (dir.length() <= 0)
        {
            dir = "/";
            Serial.printf("Empty path, starting from %s\n", dir.c_str());
        }

        list<SPIFFS_FileInfo> files = list<SPIFFS_FileInfo>();
        esp32_fileio::buildOrderedFileList(SPIFFS, dir.c_str(), filter.c_str(), 3, files);
        esp32_fileio::printFileSearchOrdered(res, &files, filter);
        // esp32_fileio::printDirOrdered(res, &files);
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
        return handlePagePart_FromFile(req, res, line, HTML_REF_CONST_MENU, (req->getHeader(HEADER_GROUP).length() > 0) ? "/T/menu_int.html" : "/T/menu_pub.html");

    return handlePagePart_FromString(req, res, line, HTML_REF_CONST_MENU, content);
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
    int idx = line.indexOf(searchString);
    if (idx >= 0)
    {
        if (!fileName.startsWith(PATH_SITE_ROOT))
            fileName = PATH_SITE_ROOT + fileName;

        // Serial.printf("\t[PagePart Parser]. Found %s in %s. Filling from %s. \n", searchString, line.c_str(), fileName.c_str());
        res->print(line.substring(0, idx).c_str());

        if (SPIFFS.exists(fileName.c_str()))
        {
            File fPagePart = SPIFFS.open(fileName.c_str());

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
            Serial.printf("File %s not found.\n", fileName.c_str());
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
                Serial.printf("Received request to delete %s\n", filePath.c_str());
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
                Serial.printf("Expected string %s. Instead found %s\n", "------------------------", name.substr(0, strlen("------------------------")).c_str());
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
            Serial.printf("Unknown POST Content-Type: %s\n", contentType.c_str());
            return;
        }
        string filename;
        while (parser->nextField())
        {
            string name = parser->getFieldName();
            // Serial.printf("Parsing field %s\n", name.c_str());
            char buf[512];
            if (name == "filename")
            {

                size_t readLength = parser->read((byte *)buf, 512);
                filename = string("/") + string(buf, readLength);
                
            }
            else if (name == "data" || name == "file")
            {
                filename = parser->getFieldFilename();
                // size_t readLength = parser->read((byte*)buf, 512);
                // string filePath = string(buf, readLength);
                // check if it comes from
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

                // Serial.printf("Uploading %s\n", filename.c_str());
                // Serial.printf("Parsing substring from idx %d\n", ++startIdx);

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
                    unsigned long startWrite = millis();
                    Serial.printf("Writing %u bytes to file [%s].. ", req->getContentLength(), filename.c_str());
                    size_t bytes = esp32_fileio::UpdateFile(filename.c_str(), parser,true);
                    savedFile = bytes > 0;
                    res->printf("<p>Saved %d bytes to %s</p>", bytes, filename.c_str());
                    Serial.printf(" done in %u ms", millis() - startWrite);
                }
            }
            else if (name == "path")
            {
                byte buf[512];
                // adding
                while (!parser->endOfField())
                {
                    memset(buf, 0, sizeof(buf));
                    size_t readLength = parser->read(buf, 512);
                    filename.append((const char *)buf);
                }
                Serial.printf("File to create: %s\n", filename.c_str());
                savedFile = esp32_fileio::CreateFile(filename.c_str());
            }
            else
            {
                res->printf("<p>Unexpected field %s</p>", name.c_str());
            }
        }
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
        Serial.printf("[ESP ROUTER]Serving page from file %s\n", req->getRequestString().c_str());
        handleFile(req, res);
    }
}

void esp32_router::handleFile(HTTPRequest *req, HTTPResponse *res)
{
    esp32_route_file_info fileInfo = esp32_route_file_info();
    fileInfo.requestPath = req->getRequestString().c_str();

    if (!getFileInfo(req, fileInfo))
    {
        handle404(req, res);
        return;
    }   
    
    esp32_fileio::writeFileToResponse(fileInfo, res);
}



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
        // vector<string> actions;
        // controllerObj->GetActions(&actions);
        // Serial.printf("[ESP ROUTER] Action not found for controller %s and action %s\n", route.controller.c_str(), route.action.c_str());
        // for(int i = 0; i < actions.size(); i++)
        //     Serial.printf("[ESP ROUTER] Action found %s\n", actions[i].c_str());
    }
    else
    {
        controllerObj->Action(req, res);
        controllerObj->controllerTemplate.ClearVariables();

        Serial.printf("[ESP ROUTER]Serving page from template %s\n", route.controller.c_str());
    }
    delete controllerObj;
}


string esp32_router::handleServiceRequest(esp32_service_route route){
    auto service = serviceFactory->createInstance(route);
    if (service == NULL)
    {
        Serial.printf("Service %s not found\n", route.service.c_str());
        return "error";
    }

    string returnValue = "";
    returnValue = service->Execute();
    delete service;
    return returnValue;
}

#endif
