import QtQuick 2.5

Item {
    property color color: "gray"
    property bool minor: false
    width: 1
    height: 20
    Canvas {
        width: parent.width
        height: parent.height
        onPaint: {
            var c = getContext("2d");
            c.fillStyle = color;
            if (minor)
                c.fillRect(0, height/4, width, height/2);
            else
                c.fillRect(0, 0, width, height);
        }
    }    
}
