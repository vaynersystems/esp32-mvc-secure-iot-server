var vs_editor;
var vs_tree;
var selectedDrive = 0; // 0 - SPIFFS, 1 - SD
var editorSessions = [];
var openTabs = [];
var activeTab = undefined;

function createFileUploader(d, f, g) {
    var h;
    var i = document.createElement("input");
    i.type = "file";
    i.multiple = false;
    i.name = "data";
    document.getElementById(d).appendChild(i);
    var j = document.createElement("input");
    j.id = "upload-path";
    j.type = "text";
    j.name = "path";
    j.defaultValue = "/";
    document.getElementById(d).appendChild(j);
    var k = document.createElement("button");
    k.innerHTML = 'Upload';
    document.getElementById(d).appendChild(k);
    var l = document.createElement("button");
    l.innerHTML = 'Create';
    document.getElementById(d).appendChild(l);
    function httpPostProcessRequest() {
        if (h.readyState == 4) {
            if (h.status != 200)
                alert("ERROR[" + h.status + "]: " + h.responseText);
            else {
                f.refreshPath(j.value)
            }
            hideLoading();
        }
    }
    function createPath(p) {
        h = new XMLHttpRequest();
        h.onreadystatechange = httpPostProcessRequest;
        var a = new FormData();
        a.append("path", p);
        a.append("drive",document.getElementById('drive').value);
        h.open("PUT", "/edit");
        h.send(a)
    }
    l.onclick = function (e) {
        if (j.value.indexOf(".") === -1)
            return;
        createPath(j.value);
        g.loadUrl(j.value)
    }
        ;
    k.onclick = function (e) {
        if (i.files.length === 0) {
            return
        }
        showLoading();
        h = new XMLHttpRequest();
        h.onreadystatechange = httpPostProcessRequest;
        var a = new FormData();
        a.append("data", i.files[0], j.value);
        a.append("drive",document.getElementById('drive').value);
        h.open("POST", "/edit");
        h.send(a)
    }
        ;
    i.onchange = function (e) {
        if (i.files.length === 0)
            return;
        var a = i.files[0].name;
        var b = /(?:\.([^.]+))?$/.exec(a)[1];
        var c = /(.*)\.[^.]+$/.exec(a)[1];
        if (typeof c !== undefined) {
            a = c
        }
        if (typeof b !== undefined) {
            if (b === "html")
                b = "htm";
            else if (b === "jpeg")
                b = "jpg";
            a = a + "." + b
        }
        if (j.value === "/" || j.value.lastIndexOf("/") === 0) {
            j.value = "/" + a
        } else {
            j.value = j.value.substring(0, j.value.lastIndexOf("/") + 1) + a
        }
    }
}
var req = new XMLHttpRequest();
function getTheme(){
    if (req.readyState == 4) {
        if (req.status == 200) {
            themeSelector.innerHTML = ''; //clear
            var c = JSON.parse(req.responseText);
            console.log('got themes', c);
            c = c.sort((a,b) => a.name < b.name ? -1 : 1);
            var e = c.length;
            var sStr = 'theme-';
            for (var i = 0; i < e; i++) {
                var o = document.createElement("option");
                var startIdx = c[i].name.lastIndexOf(sStr);
                var endIdx = c[i].name.lastIndexOf('.js');
                if (startIdx < 0 || endIdx < 0)
                    continue;

                var name = c[i].name.substring(startIdx + sStr.length, endIdx);
                o.value = name;
                o.text = name.replace('_', ' ');
                themeSelector.appendChild(o);
            }
        }
    }
}

function refreshThemes(){    
    req.onreadystatechange = getTheme;
    req.open("GET", "/list?path=/W/JS&filter=theme-", true);
    req.send(null);
}

var themeSelector = document.createElement("select");
function createEditor_ThemeSelector(themeDiv) {
    var n = document.getElementById(themeDiv);
    var la = document.createElement("span");
    la.innerText = "Theme:";
    n.appendChild(la);
    
    themeSelector.id = "theme-selector";
    themeSelector.onchange = function (ev) {
        setTheme(ev.target.value);        
    }

    n.appendChild(themeSelector);
}


var oldSpan = null;
function createTree(treeElementId, editorObject) {
    var m = document.getElementById("preview");
    var previewContainerElement = document.getElementById("preview-container");    
    var previewCloseElement = document.getElementById('preview-close');
    var n = document.createElement("div");
    var p = document.createElement("panel");
    n.className = "tvu";
    n.id = "tree-root";
    var treeElement = document.getElementById(treeElementId);
    showLoading(treeElementId);
    treeElement.style.height = (window.innerHeight - 60) + "px";
    treeElement.appendChild(n);
    function loadDownload(a) {
        document.getElementById('download-frame').src = a + "?download=true"
    }
    function loadPreview(a) {
        document.getElementById("editor").style.display = "none";
        previewContainerElement.style.visibility = 'visible';
        previewCloseElement.style.visibility = 'visible';
        m.style.display = 'block';
        m.innerHTML = '<img src="' + a + '" style="max-width:100%; max-height:100%; margin:auto; display:block;" />'
    }
    function fillFileMenu(a, b) {
        var c = document.createElement("ul");
        a.appendChild(c);
        var d = document.createElement("li");
        c.appendChild(d);
        if (isTextFile(b)) {
            d.innerHTML = "<span>Edit</span>";
            d.onclick = function (e) {
                showLoading();
                editorObject.loadUrl(b);
                var fileName = b.split('?')[1];
                document.getElementById("workingFile").innerHTML = fileName;
                updateToolbar();
                hideLoading();
            }
        } else if (isImageFile(b)) {
            d.innerHTML = "<span>Preview</span>";
            d.onclick = function (e) {
                showLoading();
                loadPreview(b);
                if (document.body.getElementsByClassName('cm').length > 0)
                    document.body.removeChild(a)
                hideLoading();
            }
        }
        var f = document.createElement("li");
        c.appendChild(f);
        f.innerHTML = "<span>Download</span>";
        f.onclick = function (e) {
            showLoading(treeElement);
            loadDownload(b);
            if (document.body.getElementsByClassName('cm').length > 0)
                document.body.removeChild(a)
            hideLoading(treeElement);
        }
            ;
        var g = document.createElement("li");
        c.appendChild(g);
        g.innerHTML = "<span>Delete</span>";
        g.onclick = function (e) {
            showLoading();
            httpDelete(b);
            if (document.body.getElementsByClassName('cm').length > 0)
                document.body.removeChild(a)
            hideLoading();
        }
    }
    function showContextMenu(e, a, b) {
        var c = document.createElement("div");
        var d = document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
        var f = document.body.scrollLeft ? document.body.scrollLeft : document.documentElement.scrollLeft;
        var g = event.clientX + f;
        var h = event.clientY + d;
        c.className = 'cm';
        c.style.display = 'block';
        c.style.left = g + 'px';
        c.style.top = h + 'px';
        fillFileMenu(c, a);
        document.body.appendChild(c);
        var i = c.offsetWidth;
        var j = c.offsetHeight;
        c.onmouseout = function (e) {
            if (e.clientX < g || e.clientX > (g + i) || e.clientY < h || e.clientY > (h + j)) {
                if (document.body.getElementsByClassName('cm').length > 0)
                    document.body.removeChild(c)
            }
        }
    }

    function createTreeLeaf(directory, filename, size, nodeType, createdDate) {
        var liElement = document.createElement("li");
        //d.id =  directory + (directory.length > 0 ? '/' : '') + filename;
        liElement.id =  directory + filename;
        var f = document.createElement("div");
        f.innerText = filename.substring(filename.lastIndexOf('/') + 1);
        
        if (nodeType == "dir") {
            f.className = "list-dir";
            f.style.paddingLeft = ((directory.split('/').length + 1) * 10) + 'px';
            f.style.backgroundPosition = ((directory.split('/').length * 10) - 5) + 'px';
            //liElement.id =  filename;
        } else{
            liElement.style.paddingLeft = ((directory.split('/').length) * 10) + 'px';
        }

        
        liElement.appendChild(f);
        liElement.onclick = function (e) {

            if (isTextFile(liElement.id.toLowerCase())) {
                showLoading();
                editorObject.loadUrl(liElement.id)
                // liElement.classList.add("viewing");
                // if (oldSpan !== null)
                //     oldSpan.classList.remove("viewing");
                // oldSpan = liElement;
                
                var workingFileElement = document.getElementById("workingFile");
                if(workingFileElement !== undefined)
                    workingFileElement.innerHTML = liElement.id;
                editorObject.updateToolbar();
                //att.value = "file";
            } else if (isImageFile(liElement.id.toLowerCase())) {
                loadPreview(liElement.id);
                // att.value = "binary";
            } else 
                toggleDir(liElement);
            //todo: add for directory
            //d.setAttributeNode(att)
            e.cancelBubble = true;

        };

        if (nodeType != "dir") {
            liElement.oncontextmenu = function (e) {
                e.preventDefault();
                e.stopPropagation();
                showContextMenu(e, liElement.id, true)
            }
        }
            ;
        return liElement;
    }
    function ensurePathExists(path){
        var paddingLeftPixels = 10;
        var rootNode = treeElement.querySelector("ul");
        const pathParts = path.split('/').filter( p => p.length > 0);
        //pathParts.pop(); // remove file name from path
        let lastPath = '';
        for(const part of pathParts){
            const foundElement = document.getElementById(lastPath + '/' + part);
            if(foundElement !== null) {
                lastPath += '/' + part;
                continue;
            }

            //need to add element. If no lastPath, its root. otherwise add to lastPath
            const parentElement = document.getElementById(lastPath);
            const child = createTreeLeaf(lastPath, '/' + part, 0, 'dir', '');
            if(parentElement == null)
                rootNode.appendChild(child);    // add to root
            else {
                // var paddingLeft = 0;//parentElement.style.paddingLeft === undefined ? 0 :  parseInt(parentElement.style.paddingLeft.replace('px', ''));
                // child.style.paddingLeft = ((isNaN(paddingLeft) ? 0 : paddingLeft) + paddingLeftPixels) + "px";
                parentElement.appendChild(child);
            }
                
            lastPath += '/' + part;
        }
    }
    //root Path should let us list files that are not at / but at /something/else
    function addList(treeElementId, rootPath, fileList) {
        var paddingLeftPixels = 10;
        var rootNode = document.createElement("ul");
        
        treeElementId.innerHTML = ''; //clear
        treeElementId.appendChild(rootNode);

        // var dirs = fileList
        //     .filter((v,idx, list) => {
        //         return v.type == 'dir' && list.findIndex(e => e.name === v.name) === idx;
        //     })
        //     .sort((a,b) => {if(a.name < b.name) return -1; if(b.name < a.name) return 1; return 0;});
        var dirs = fileList
            .map(fl => Object.assign({'name': fl.parent_dir }))
            .filter((v,idx, list) => {
                return list.findIndex(e => e.name === v.name) === idx;
            })
            .sort((a,b) => {if(a.name < b.name) return -1; if(b.name < a.name) return 1; return 0;});
        for(var dir of dirs)
            ensurePathExists(dir.name);

        
        var files = fileList
            .filter((v,idx, list) => {
                return v.type == 'file' && list.findIndex(e => e.name === v.name && e.parent_dir == v.parent_dir) === idx;
            })
            .sort((a,b) => {if(a.name < b.name) return -1; if(b.name < a.name) return 1; return 0;});
        
        var fileCount = files.length;
        for (var i = 0; i < fileCount; i++) {
           //ensureParentsExist(files[i].parent_dir);
            var parentPath = files[i].parent_dir;
            var parentElement = document.getElementById(parentPath);
            var child = createTreeLeaf(files[i].parent_dir, '/' + files[i].name, files[i].size, files[i].type, files[i].last_modified);
            if (parentElement !== null) {
                var level = parentPath.split('/').length;                
                parentElement.appendChild(child);
            }
        }
    }
    function isTextFile(a) {
        var b = /(?:\.([^.]+))?$/.exec(a)[1];
        if (typeof b !== undefined) {
            switch (b) {
                case "txt":
                case "htm":
                case "js":
                case "c":
                case "h":
                case "hpp":
                case "cs":
                case "java":
                case "sql":
                case "php":
                case "cpp":
                case "css":
                case "xml":
                case "":
                case "html":
                case "json":
                case "README":
                case "me":
                case "log":
                    return true
            }
        }
        return false
    }
    function isImageFile(a) {
        var b = /(?:\.([^.]+))?$/.exec(a)[1];
        if (typeof b !== undefined) {
            switch (b) {
                case "png":
                case "jpg":
                case "jpeg":
                case "gif":
                case "bmp":
                case "tiff":
                case "webp":
                case "webm":

                    return true
            }
        }
        return false
    }
    this.refreshPath = function () {
        n.innerHTML = '';
        httpGet(n, "/")
    }
        ;
    function delCb(a) {
        return function () {
            if (xmlHttp.readyState == 4) {
                if (xmlHttp.status != 200) {
                    alert("ERROR[" + xmlHttp.status + "]: " + xmlHttp.responseText)
                } else {
                    n.removeChild(n.childNodes[0]);
                    httpGet(n, "/")
                }
            }
        }
    }
    function httpDelete(filePath) {
        xmlHttp = new XMLHttpRequest();
        xmlHttp.onreadystatechange = delCb(filePath);
        var formData = new FormData();
        formData.append("path", filePath);
        xmlHttp.open("DELETE", "/edit");
        xmlHttp.send(formData)
    }
    function getCb(treeElementId, rootNode) {
        return function () {
            if (xmlHttp.readyState == 4) {
                if (xmlHttp.status == 200)
                    addList(treeElementId, rootNode, JSON.parse(xmlHttp.responseText))
                hideLoading();
            }
        }
    }
    function httpGet(treeElementId, searchPath) {
        const drive = document.getElementById('drive').value;
        xmlHttp = new XMLHttpRequest(treeElementId, searchPath);
        xmlHttp.onreadystatechange = getCb(treeElementId, searchPath);
        xmlHttp.open("GET", "/list?dir=" + searchPath + "&drive=" + drive, true);
        xmlHttp.send(null)
    }

    this.selectElement = function(elementPath){
        const liElement = document.getElementById(elementPath);
        if(liElement === undefined || liElement === null) return;
        liElement.classList.add("viewing");
        if (oldSpan !== null && oldSpan !== liElement)
            oldSpan.classList.remove("viewing");
        oldSpan = liElement;
        liElement.scrollIntoView({
            behavior: "smooth", // Optional for smooth scrolling
            block: 'nearest',
            inline: 'start',
        });       
    }
    
    httpGet(n, "/");
    hideLoading();
    return this
}
function createEditor(container, path, g, theme, i) {

    function fillFileMenu(a, b) {
        var c = document.createElement("ul");
        a.appendChild(c);
        var d = document.createElement("li");
        c.appendChild(d);
        if (isTextFile(b)) {
            d.innerHTML = "<span>Edit</span>";
            d.onclick = function (e) {
                showLoading();
                editorObject.loadUrl(b);
                var fileName = b.split('?')[1];
                document.getElementById("workingFile").innerHTML = fileName;
                updateToolbar();
                hideLoading();
            }
        } else if (isImageFile(b)) {
            d.innerHTML = "<span>Preview</span>";
            d.onclick = function (e) {
                showLoading();
                loadPreview(b);
                if (document.body.getElementsByClassName('cm').length > 0)
                    document.body.removeChild(a)
                hideLoading();
            }
        }
        var f = document.createElement("li");
        c.appendChild(f);
        f.innerHTML = "<span>Download</span>";
        f.onclick = function (e) {
            showLoading(treeElement);
            loadDownload(b);
            if (document.body.getElementsByClassName('cm').length > 0)
                document.body.removeChild(a)
            hideLoading(treeElement);
        }
            ;
        var g = document.createElement("li");
        c.appendChild(g);
        g.innerHTML = "<span>Delete</span>";
        g.onclick = function (e) {
            showLoading();
            httpDelete(b);
            if (document.body.getElementsByClassName('cm').length > 0)
                document.body.removeChild(a)
            hideLoading();
        }
    }
    function showContextMenu(e, a, b) {
        var c = document.createElement("div");
        var d = document.body.scrollTop ? document.body.scrollTop : document.documentElement.scrollTop;
        var f = document.body.scrollLeft ? document.body.scrollLeft : document.documentElement.scrollLeft;
        var g = event.clientX + f;
        var h = event.clientY + d;
        c.className = 'cm';
        c.style.display = 'block';
        c.style.left = g + 'px';
        c.style.top = h + 'px';
        fillFileMenu(c, a);
        document.body.appendChild(c);
        var i = c.offsetWidth;
        var j = c.offsetHeight;
        c.onmouseout = function (e) {
            if (e.clientX < g || e.clientX > (g + i) || e.clientY < h || e.clientY > (h + j)) {
                if (document.body.getElementsByClassName('cm').length > 0)
                    document.body.removeChild(c)
            }
        }
    }

    function getLangFromFilename(a) {
        var b = "plain";
        var c = /(?:\.([^.]+))?$/.exec(a)[1];
        if (typeof c !== undefined) {
            switch (c) {
                case "txt":
                    b = "plain";
                    break;
                case "htm":
                case "html":
                    b = "html";
                    break;
                case "js":
                    b = "javascript";
                    break;
                case "c":
                case "h":
                case "hpp":
                case "cpp":
                    b = "c_cpp";
                    break;
                case "css":
                case "scss":
                    b = "css";
                case "sql":
                    b = "sql";
                case "php":
                case "json":
                case "xml":
                    b = c;
            }
        }
        return b
    }
    if (typeof path === "undefined")
        path = "/index.html";
    if (typeof g === "undefined") {
        g = getLangFromFilename(path)
    }
    if (typeof theme === "undefined")
        theme = "textmate";
    if (typeof i === "undefined") {
        i = "text/" + g;
        if (g === "c_cpp")
            i = "text/plain"
    }
    var j = null;
    if (typeof ace === 'undefined') {
    //if(null === ace || undefined=== ace ){
        alert('Critical components failed to load. Refreshing page...');
        if(location.hostname !== 'localhost')
            location.reload();
        else return;
    }
    var editor = ace.edit(container);    
    //editorTheme= 'chrome';
    //require('ace/ext/searchbox');
    //ace.config.loadModule("ace/ext/searchbox", function(m) {m.Search(editor)});

    var UndoManager = ace.UndoManager;


    var currentSession = editorSessions.find(es => es.url == path);    
    if(currentSession === undefined){
        currentSession = {'url': path, 'session': ace.createEditSession("")};
        editorSessions.push(currentSession);
    }
     
    editor.setSession(currentSession.session);

    // editor.oncontextmenu = function (e) {
    //     e.preventDefault();
    //     e.stopPropagation();
    //     showContextMenu(e, container.id, true)
    // }
    editor.addEventListener("contextmenu", function(e) {
        e.preventDefault();
        alert('success!');
        showContextMenu(e, editor.id, true)
        return false;
    }, false);
    
    var refs = {};
    editor.updateToolbar = function updateToolbar() {
        refs.saveButton.disabled = editor.session.getUndoManager().isClean();
        refs.cancelButton.disabled = editor.session.getUndoManager().isClean();
        refs.undoButton.disabled = !editor.session.getUndoManager().hasUndo();
        refs.redoButton.disabled = !editor.session.getUndoManager().hasRedo();
        refs.refreshButton.disabled = editor.session.getDocument() == null;
    }
    editor.on("input", editor.updateToolbar);

    function save() {
        editor.save();
        //localStorage.savedValue = editor.getValue();
        editor.session.getUndoManager().markClean();
        editor.updateToolbar();
    }
    editor.commands.addCommand({
        name: "save",
        exec: save,
        bindKey: { win: "ctrl-s", mac: "cmd-s" }
    });

    editor.commands.addCommand({
        name:"search",
        exec: () => editor.execCommand('find'),
        bindKey: { win: "ctrl-f", mac: "cmd-f" }
    })
    editor.commands.addCommand({
        name: "tab left",
        exec: () => SetTabPrevious(),
        bindKey: { win: "Alt-[", mac: "cmd-[" }
    });
    editor.commands.addCommand({
        name: "tab right",
        exec: () => setTabNext(),
        bindKey: { win: "Alt-]", mac: "cmd-]" }
    });
    refs.cancelButton = document.getElementById("workingFile_btnCancel");
    refs.saveButton = document.getElementById("workingFile_btnSave");
    refs.undoButton = document.getElementById("workingFile_btnUndo");
    refs.redoButton = document.getElementById("workingFile_btnRedo");
    refs.refreshButton = document.getElementById("workingFile_btnRefresh");
    refs.refreshThemesButton = document.getElementById("theme_refresh");
    

    refs.saveButton.addEventListener("click", function () {
        save();
    });
    refs.cancelButton.addEventListener("click", function () {
        editor.loadUrl(container);
    });
    refs.undoButton.addEventListener("click",
        function () {
            editor.undo();
        });
    refs.redoButton.addEventListener("click", function () {
        editor.redo();
    });

    refs.refreshButton.addEventListener("click", function(){
        httpGet(path); //get new doc
        editor.loadUrl(path); //load doc and plugins 
    });

    refs.refreshThemesButton.addEventListener("click", function(){
        refreshThemes();
    });
    //var o = document.getElementById(e);
    //o.appendChild(k);

    window.editor = editor;
    

    function httpPostProcessRequest() {
        if (j.readyState == 4) {
            if (j.status != 200)
                alert("ERROR[" + j.status + "]: " + j.responseText)
            hideLoading();
        }
    }
    function httpPost(a, b, c) {
        j = new XMLHttpRequest();
        j.onreadystatechange = httpPostProcessRequest;
        var d = new FormData();
        d.append("data", new Blob([b], {
            type: c
        }), a);
        j.open("POST", "/edit");
        j.send(d)
    }
    function httpGetProcessRequest() {
        if (j.readyState == 4) {
            //document.getElementById("preview").style.display = "none";
            // editor.url = path;
            document.getElementById("editor").style.display = "block";
            if(currentSession === undefined){
                currentSession = {'url': path, 'session': ace.createEditSession("")};
                editorSessions.push(currentSession);
            }
            editor.setSession(currentSession.session);
            setTab(path);

            if (j.status == 200){
                editor.setValue(j.responseText);
            }
            else
                editor.setValue("");
            editor.clearSelection();
            hideLoading();
        }
    }
    function httpGet(url) {
        const drive = document.getElementById('drive').value;
        if(url.indexOf('?') > 0)
            url = url + '&drive=' + drive;
        else
            url = url + '?drive=' + drive;
        j = new XMLHttpRequest();
        j.onreadystatechange = httpGetProcessRequest;
        j.open("GET", url, true);
        j.setRequestHeader('Cache-Control','no-cache');
        j.send(null)
    }
    if (g !== "plain")
        editor.getSession().setMode("ace/mode/" + g);
    

    editor.setOptions({
        theme: "ace/theme/" + theme,
        mode: "ace/mode/html",
        minLines: 30,
        autoScrollEditorIntoView: true,
    });

    editor.$blockScrolling = Infinity;
    editor.getSession().setUseSoftTabs(true);
    editor.getSession().setTabSize(2);
    editor.setHighlightActiveLine(true);
    editor.setShowPrintMargin(false);
    editor.commands.addCommand({
        name: 'saveCommand',
        bindKey: {
            win: 'Ctrl-S',
            mac: 'Command-S'
        },
        exec: function (a) {
            showLoading();
            httpPost(path, a.getValue() + "", i)
        },
        readOnly: false
    });
    editor.save = function () {
        showLoading();
        httpPost(path, editor.getValue() + "", i);
    };
    

    editor.loadUrl = function (a) {
        //first check if the current editor has a session that needs to be saved
        const curSession = editorSessions.find(s => s.url == path);
        if(curSession !== undefined){
            //session exists
            curSession.document = editor.getValue();
            //curSession.session.setValue(editor.getValue());
        }
        path = a;
        var isNewEditorSession = false;
        var currentSession = editorSessions.find(es => es.url == path);    
        if(currentSession === undefined){
            currentSession = {'url': path, 'session': ace.createEditSession("")};
            currentSession.session.undoManager = new UndoManager();
            editorSessions.push(currentSession);
            isNewEditorSession = true;
        }
        editor.setSession(currentSession.session);
        editor.session.setUndoManager(currentSession.session.undoManager);
        g = getLangFromFilename(path);
        i = "text/" + g;
        if (g !== "plain")
            editor.getSession().setMode("ace/mode/" + g);
        
        if(isNewEditorSession) httpGet(path) //load only if not already loaded
        else {
            editor.clearSelection();
            
            editor.session.undoManager = currentSession.session.undoManager;
            editor.getSession().setValue(currentSession.document);
            setTab(path);
            document.getElementById("editor").style.display = "block";
            hideLoading();
            
        }

        // var undoManager = new UndoManager();
        // editor.getSession().setUndoManager(undoManager);
        
    
        document.getElementById("workingFile").innerHTML = path;
        editor.updateToolbar();
            
    };

    //editor.loadUrl(path);
    
    editor.on('change', function (e) {
        hideLoading();
    });

    function setTab(path){
        const tab = openTabs.find(t => t.url == path);
        if(tab === null || tab === undefined)
            _addTab(path);
        
        _selectTab(path);
    }
    function SetTabPrevious(){
        var active = openTabs.find(t => t.url == activeTab.title);
        var activeIdx = openTabs.indexOf(active);
        if(activeIdx === undefined) return;
        if(activeIdx <= 0) return;
        editor.loadUrl(openTabs[activeIdx - 1].url);
        vs_tree.selectElement(openTabs[activeIdx - 1].url);
    }
    function setTabNext(){
        var activeIdx = openTabs.find(t => t.url == activeTab.title)?.index;
        if(activeIdx === undefined) return;        
        if(activeIdx >= openTabs.length - 1) return;
        editor.loadUrl(openTabs[activeIdx + 1].url);
        vs_tree.selectElement(openTabs[activeIdx + 1].url);
    }

    function _selectTab(path){
        const tab = openTabs.find(t => t.url == path);
        const existingTabs = document.getElementsByClassName('tab');
        if(existingTabs === undefined || existingTabs === null || existingTabs.length == 0)
            return;
        for(var element of existingTabs){
            if(element.title !== path)
                element.removeAttribute('selected');
            else {
                element.setAttribute('selected','');
                activeTab = element;
                element.scrollIntoView();
            }
        };
        vs_tree.selectElement(path);


    }
    function _addTab(path){
        var tab = {url: path, index: openTabs.length, name: path.substring(path.lastIndexOf('/') + 1)};
        openTabs.push(tab);

        const tabsElement = document.getElementById('tabs');
        const tabElement = document.createElement('div');
        tabElement.title = tab.url;
        tabElement.className = 'tab';
        tabElement.innerHTML = tab.name;
        tabElement.addEventListener('click', (ev) => {editor.loadUrl(ev.currentTarget.title)});        

        const tabElementClose = document.createElement('i');
        tabElementClose.className = "fa-solid fa-circle-xmark";
        tabElementClose.classList.add('tab-close');
        tabElementClose.title = 'Close ' + tab.name;
        tabElementClose.addEventListener('click', () => {event.stopPropagation(); _removeTab(path); });
        tabElement.appendChild(tabElementClose);
        tabsElement.appendChild(tabElement);
        editor.loadUrl(tab.url);

    }

    function _removeTab(path){
        const tab = openTabs.find(t => t.url == path);
        const tabElement = document.querySelector("#tabs .tab[title='" + path + "']");
        if(tab === undefined && tabElement === undefined){
            showModal('Error','error removing tab');
            return;
        }
        const tabIdx = openTabs.indexOf(tab);
        
        openTabs = openTabs.filter(t => t.url !== tab.url);
        tabElement.parentElement.removeChild(tabElement);
        editorSessions = editorSessions.filter( es => es.url !== tab.url);
        if(activeTab.title === tab.url){
            //removing active tab, where to focus?
            
            if( tabIdx < openTabs.length - 1){
                //not last item, select the next item
                _selectTab(openTabs[tabIdx + 1].url);
                editor.loadUrl(openTabs[tabIdx + 1].url);
            } else if(tabIdx > 0){
                _selectTab(openTabs[tabIdx - 1].url);
                editor.loadUrl(openTabs[tabIdx - 1].url);
            }
            else{
                hideEditor();
            }

        }

    }
    setTheme(theme);

    
    return editor;
}

function showLoading() {
    var l = document.getElementById('loading');
    if (l !== null) l.style.visibility = 'visible';
}
function hideLoading() {
    var l = document.getElementById('loading');
    if (l !== null) l.style.visibility = 'hidden';
}
function hideEditor(){
    var editor = document.getElementById('editor');
    if (editor !== null) editor.style.display = 'none';
}

function prettyBytes(size){
    if (size === undefined) return "";
    return size > 1024*1024*1024 ? (size / (1024*1024*1024)).toFixed(2) + "GB" :
        size > 1024*1024 ? (size / (1024*1024)).toFixed(2) + "MB" :
        size > 1024 ? (size / (1024)).toFixed(2) + "KB" :
        size.toFixed(2) + "B"
}

function onBodyLoad() {

    
    var c = { 'file': 'index.html', 'lang': 'html', 'theme': 'textmate' };
    var d = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, a, b) {
        c[a] = b
    });
    vs_editor = createEditor("editor", c.file, c.lang, c.theme);
    vs_tree = createTree("tree", vs_editor);
    createFileUploader("uploader", vs_tree, vs_editor);
    createEditor_ThemeSelector("themes");

    const themeSelectorElement = document.getElementById('themeSelector');
    if(themeSelectorElement !== undefined && themeSelectorElement !== null)
        themeSelectorElement.value = c.theme;
    
    //splitter
    dragElement(document.getElementById("seperator"), "H");

    const treeRefresh = document.getElementById('tree-refresh');
    if(treeRefresh !== null){
        treeRefresh.addEventListener('click', function() { 
            showLoading();
            n = document.getElementById('tree-root');
            if(n!== undefined){
                vs_tree.refreshPath();
            }                
            
        });
    }

    const driveSelectorElement = document.getElementById('drive');
    driveSelectorElement.innerHTML = '';
    if(driveSelectorElement !== null){
        driveSelectorElement.addEventListener('change',(ev) => {
            selectedDrive = ev.target.value;
            vs_tree.refreshPath();
            driveLabelElement = document.getElementById('drive-name');
            driveSizeElement = document.getElementById('drive-size');
            driveLabelElement.innerHTML = ev.target.selectedOptions[0].tag.name;
            var used = ev.target.selectedOptions[0].tag.used ;
            var total = ev.target.selectedOptions[0].tag.size;
            driveSizeElement.style.setProperty('--percent', ((used/ total) * 100).toFixed(0));
            driveSizeElement.title = 'Used ' + prettyBytes(used) + ' of ' + prettyBytes(total);
        })
    }

    //load drives
    function loadDrives(){
        xmlHttp = new XMLHttpRequest();
        xmlHttp.onreadystatechange = () =>{
        
            if (xmlHttp.readyState == 4) {
                if (xmlHttp.status == 200)
                {
                    var drives = JSON.parse(xmlHttp.responseText);
                    for(var driveConfig of drives){
                        const driveOptionElement = document.createElement('option');
                        const driveSize =  prettyBytes(driveConfig.size);
                        driveOptionElement.value = driveConfig.index;
                        driveOptionElement.text =  driveConfig.name + " - "  + driveSize;
                        driveOptionElement.tag = driveConfig;
                        driveSelectorElement.appendChild(driveOptionElement);
                    }

                    selectedDrive = driveSelectorElement.value;
                    vs_tree.refreshPath();
                    var dr = drives[selectedDrive];
                    driveLabelElement = document.getElementById('drive-name');
                    driveSizeElement = document.getElementById('drive-size');
                    driveLabelElement.innerHTML = dr.name;
                    driveSizeElement.style.setProperty('--percent', ((dr.used / dr.size) * 100).toFixed(0));
                    driveSizeElement.title = 'Used ' + prettyBytes(dr.used) + ' of ' + prettyBytes(dr.size);
                }
            }        
        };
        xmlHttp.open("GET", "/esp32_system_info/Disks", true);
        xmlHttp.send(null);
    }

    loadDrives();

    refreshThemes();
    
};

screen.onload = function () {
    onBodyLoad();
};

function hidePreview(){
    var previewElement = document.getElementById('preview-container');
    var previewCloseElement = document.getElementById('preview-close');
    if(previewElement === undefined)
        return;
    previewElement.style.visibility = 'hidden';
    if(previewCloseElement === undefined)
        return;
    previewCloseElement.style.visibility = 'hidden';
}

// function is used for dragging and moving
function dragElement(element, direction, handler) {
    // Two variables for tracking positions of the cursor
    var bodyRect = document.body.getBoundingClientRect();
    const drag = { x: bodyRect.left, y: bodyRect.top };
    const delta = { x: bodyRect.left, y: bodyRect.top };

    /* if present, the handler is where you move the DIV from
       otherwise, move the DIV from anywhere inside the DIV */
    handler ? (handler.onmousedown = dragMouseDown) : (element.onmousedown = dragMouseDown);

    // function that will be called whenever the down event of the mouse is raised
    function dragMouseDown(e) {
        //var bodyRect = document.body.getBoundingClientRect();
        /*,
      elemRect = element.getBoundingClientRect(),
      offset   = elemRect.top - bodyRect.top;
      */
        drag.x = e.clientX - bodyRect.x;
        drag.y = e.clientY - bodyRect.y;
        document.onmousemove = onMouseMove;
        document.onmouseup = () => { document.onmousemove = document.onmouseup = null; }
    }

    // function that will be called whenever the up event of the mouse is raised
    function onMouseMove(e) {
        const currentX = e.clientX;
        const currentY = e.clientY;

        delta.x = currentX - drag.x;
        delta.y = currentY - drag.y;
        //console.log('Offset from ' + drag.x + ' to ' + currentX);
        const offsetLeft = element.offsetLeft;
        const offsetTop = element.offsetTop;


        const first = document.getElementById("tree");
        const second = document.getElementById("editor");
        let firstWidth = first.offsetWidth - bodyRect.x;
        let secondWidth = second.offsetWidth;
        if (direction === "H") // Horizontal
        {
            element.style.left = offsetLeft + delta.x + "px";
            firstWidth += delta.x;
            secondWidth -= delta.x;
        }
        drag.x = currentX;
        drag.y = currentY;
        first.style.width = firstWidth + "px";
        second.style.width = secondWidth + "px";
    }

}


function setTheme(theme){
    showLoading();
    editor.editorTheme = theme;
    editor.setTheme("ace/theme/" + theme);
    setTimeout( () => {
        var treeElement = document.getElementById('tree');
        var editorElement = document.getElementById('editor');
        if(treeElement !== undefined && editorElement !== undefined)
            tree.className = editorElement.className;
        hideLoading();
    }, 1000);
}
    
function toggleDir(dirElement){
    if (dirElement.childElementCount > 1 /* == "dir"*/) {
        innerTag = dirElement.childNodes[0];
        //toggle collapse/expand
        if (dirElement.classList.contains("collapsed")) {
            //expand
            dirElement.classList.remove("collapsed");
            innerTag.classList.add("list-dir");
        } else {
            dirElement.classList.add("collapsed");
            innerTag.classList.remove("list-dir");
        }
    }
}