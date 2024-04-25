var vs_editor;
var vs_tree;

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

var themeSelector = document.createElement("select");
function createEditor_ThemeSelector(themeDiv) {
    var n = document.getElementById(themeDiv);
    var la = document.createElement("span");
    la.innerText = "Theme:";
    n.appendChild(la);
    

    themeSelector.onchange = function (ev) {
        setTheme(ev.target.value);        
    }

    refreshThemes();

    n.appendChild(themeSelector);


}
function refreshThemes(){
    var req = new XMLHttpRequest();
    req.onreadystatechange = function (ev) {
        if (xmlHttp.readyState == 4) {
            if (xmlHttp.status == 200) {
                themeSelector.innerHTML = ''; //clear
                var c = JSON.parse(xmlHttp.responseText);
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
    req.open("GET", "/list?path=/W/JS&filter=theme-", true);
    req.send(null);
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
    var t = document.getElementById(treeElementId);
    showLoading(t);
    t.style.height = (window.innerHeight - 60) + "px";
    t.appendChild(n);
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
            showLoading(t);
            loadDownload(b);
            if (document.body.getElementsByClassName('cm').length > 0)
                document.body.removeChild(a)
            hideLoading(t);
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

    function createTreeLeaf(a, b, size, nodeType, createdDate) {
        var d = document.createElement("li");
        d.id = (((a == "/") ? window.document.URL + "?" : a) + b);
        var f = document.createElement("div");
        f.innerText = b.substring(b.lastIndexOf('/') + 1);
        
        if (nodeType == "dir") {
            f.className = "list-dir";
        }

        d.appendChild(f);
        d.onclick = function (e) {

            if (isTextFile(d.id.toLowerCase())) {
                showLoading();
                editorObject.loadUrl(d.id)
                d.classList.add("viewing");
                if (oldSpan !== null)
                    oldSpan.classList.remove("viewing");
                oldSpan = d;
                
                var workingFileElement = document.getElementById("workingFile");
                if(workingFileElement !== undefined)
                    workingFileElement.innerHTML = d.id;
                editorObject.updateToolbar();
                //att.value = "file";
            } else if (isImageFile(d.id.toLowerCase())) {
                loadPreview(d.id);
                // att.value = "binary";
            } else 
                toggleDir(d);
            //todo: add for directory
            //d.setAttributeNode(att)
            e.cancelBubble = true;

        };

        if (nodeType != "dir") {
            d.oncontextmenu = function (e) {
                e.preventDefault();
                e.stopPropagation();
                showContextMenu(e, d.id, true)
            }
        }
            ;
        return d;
    }
    function addList(treeElementId, rootPath, fileList) {
        var left_margin = 10;
        var rootNode = document.createElement("ul");
        
        treeElementId.innerHTML = ''; //clear
        treeElementId.appendChild(rootNode);

        var dirs = fileList
            .filter((v,idx, list) => {
                return v.type == 'dir' && list.findIndex(e => e.name === v.name) === idx;
            })
            .sort((a,b) => a.name > b.name);

        var dirCount = dirs.length;
        for (var i = 0; i < dirCount; i++) {
           
            var parentPath = dirs[i].name.substring(0,dirs[i].name.lastIndexOf('/'));
            var parentElement = document.getElementById(parentPath);
            var child = createTreeLeaf(dirs[i].parent_dir, dirs[i].name, dirs[i].size, dirs[i].type, dirs[i].last_modified);
            if (parentElement !== null) {
                var marginleft = parseInt(parentElement.style.marginLeft.replace('px', ''));
                child.style.marginLeft = (marginleft + left_margin) + "px";
                parentElement.appendChild(child);
            } else {
                child.style.marginLeft = "0px";
                rootNode.appendChild(child);
            }
        }

        var files = fileList
            .filter((v,idx, list) => {
                return v.type == 'file' && list.findIndex(e => e.name === v.name) === idx;
            })
            .sort((a,b) => a.name > b.name);
        
        var fileCount = files.length;
        for (var i = 0; i < fileCount; i++) {
           
            var parentPath = files[i].parent_dir;
            var parentElement = document.getElementById(parentPath);
            var child = createTreeLeaf(files[i].parent_dir, files[i].name, files[i].size, files[i].type, files[i].last_modified);
            if (parentElement !== null) {
                var marginleft = parseInt(parentElement.style.marginLeft.replace('px', ''));
                child.style.marginLeft = (marginleft + left_margin) + "px";
                parentElement.appendChild(child);
            } else {
                child.style.marginLeft = "0px";
                rootNode.appendChild(child);
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
                case "README":
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
        xmlHttp = new XMLHttpRequest(treeElementId, searchPath);
        xmlHttp.onreadystatechange = getCb(treeElementId, searchPath);
        xmlHttp.open("GET", "/list?dir=" + searchPath, true);
        xmlHttp.send(null)
    }
    
    httpGet(n, "/");
    hideLoading();
    return this
}
function createEditor(e, f, g, h, i) {
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
    if (typeof f === "undefined")
        f = "/index.html";
    if (typeof g === "undefined") {
        g = getLangFromFilename(f)
    }
    if (typeof h === "undefined")
        h = "textmate";
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
    var editor = ace.edit(e);
    editorTheme= 'tomorrow_night';
    require('ace/ext/searchbox');
    
    
    var refs = {};
    editor.updateToolbar = function updateToolbar() {
        refs.saveButton.disabled = editor.session.getUndoManager().isClean();
        refs.cancelButton.disabled = editor.session.getUndoManager().isClean();
        refs.undoButton.disabled = !editor.session.getUndoManager().hasUndo();
        refs.redoButton.disabled = !editor.session.getUndoManager().hasRedo();
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
    refs.cancelButton = document.getElementById("workingFile_btnCancel");
    refs.saveButton = document.getElementById("workingFile_btnSave");
    refs.undoButton = document.getElementById("workingFile_btnUndo");
    refs.redoButton = document.getElementById("workingFile_btnRedo");
    refs.refreshButton = document.getElementById("theme_refresh");
    

    refs.saveButton.addEventListener("click", function () {
        save();
    });
    refs.cancelButton.addEventListener("click", function () {
        editor.loadUrl(e);
    });
    refs.undoButton.addEventListener("click",
        function () {
            editor.undo();
        });
    refs.redoButton.addEventListener("click", function () {
        editor.redo();
    });

    refs.refreshButton.addEventListener("click", function(){
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
            document.getElementById("editor").style.display = "block";
            if (j.status == 200)
                editor.setValue(j.responseText);
            else
                editor.setValue("");
            editor.clearSelection()
        }
    }
    function httpGet(a) {
        j = new XMLHttpRequest();
        j.onreadystatechange = httpGetProcessRequest;
        j.open("GET", a, true);
        j.setRequestHeader('Cache-Control','no-cache');
        j.send(null)
    }
    if (g !== "plain")
        editor.getSession().setMode("ace/mode/" + g);
    

    editor.setOptions({
        theme: "ace/theme/" + editorTheme,
        mode: "ace/mode/html",
        minLines: 30,
        autoScrollEditorIntoView: true,
    });
    setTheme(h);
    
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
            httpPost(f, a.getValue() + "", i)
        },
        readOnly: false
    });
    editor.save = function () {
        showLoading();
        httpPost(f, editor.getValue() + "", i);
    };
    editor.commands.addCommand({
        name: 'undoCommand',
        bindKey: {
            win: 'Ctrl-Z',
            mac: 'Command-Z'
        },
        exec: function (a) {
            a.getSession().getUndoManager().undo(false)
        },
        readOnly: false
    });
    editor.commands.addCommand({
        name: 'redoCommand',
        bindKey: {
            win: 'Ctrl-Shift-Z',
            mac: 'Command-Shift-Z'
        },
        exec: function (a) {
            a.getSession().getUndoManager().redo(false)
        },
        readOnly: false
    });
    httpGet(f);
    editor.loadUrl = function (a) {
        f = a;
        g = getLangFromFilename(f);
        i = "text/" + g;
        if (g !== "plain")
            editor.getSession().setMode("ace/mode/" + g);
        httpGet(f)
    };
    editor.on('change', function (e) {
        hideLoading();
    });
    // editor.renderer.on('afterRender', function (component, helper) {
    //     console.log(editor.getHighlightActiveLine());
    // });
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

function onBodyLoad() {
    //$(window).load(function() {
    //$('#loading').hide();
    //});

    var c = { 'file': 'index.html', 'lang': 'html', 'theme': 'tomorrow_night' };
    var d = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, a, b) {
        c[a] = b
    });
    vs_editor = createEditor("editor", c.file, c.lang, c.theme);
    vs_tree = createTree("tree", vs_editor);
    createFileUploader("uploader", vs_tree, vs_editor);
    createEditor_ThemeSelector("themes");

    //splitter
    dragElement(document.getElementById("seperator"), "H");

    var treeRefresh = document.getElementById('tree-refresh');
    if(treeRefresh != undefined){
        treeRefresh.addEventListener('click', function() { 
            showLoading();
            n = document.getElementById('tree-root');
            if(n!== undefined){
                vs_tree.refreshPath();
            }                
            
        });
    }
    
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
        innerTag = dirElement.child(0);
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