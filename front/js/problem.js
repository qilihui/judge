function keyHandler(obj) {
    // alert(1);
    var TABKEY = 9;
    // alert(event.keyCode);
    if (event.keyCode == TABKEY) {
        // alert(1341423);
        insertText(obj);
        if (event.preventDefault) {
            event.preventDefault();
        }
    }
}
function insertText(obj) {
    str = "    ";
    if (document.selection) {
        var sel = document.selection.createRange();
        sel.text = str;
    } else if (typeof obj.selectionStart === 'number' && typeof obj.selectionEnd === 'number') {
        var startPos = obj.selectionStart,
            endPos = obj.selectionEnd,
            cursorPos = startPos,
            tmpStr = obj.value;
        obj.value = tmpStr.substring(0, startPos) + str + tmpStr.substring(endPos, tmpStr.length);
        cursorPos += str.length;
        obj.selectionStart = obj.selectionEnd = cursorPos;
    } else {
        obj.value += str;
    }
}
