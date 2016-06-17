import QtQuick 2.5
import QtQuick.Layouts 1.3

// A gauge that displays a value on a zero-centered axis with tick marks
Item {
    property real value: 0
    property real limit: 50.0
    property color barColor: "lime"
    property color axisColor: "gray"
    property int numTicks: 5
    property int tickHeight: 20
    height: childrenRect.height
    width: 150
    Canvas {
        id: axis
        height: 1
        width: parent.width
        anchors.top: parent.top
        onPaint: {
            var c = getContext("2d");
            c.fillStyle = axisColor;
            c.fillRect(0, 0, width, 1);
        }
    }
    Repeater {
        model: numTicks
        TickMark {
            height: tickHeight
            x: index * axis.width / (numTicks - 1)
            minor: index % 2 != 0
            anchors.verticalCenter: axis.verticalCenter
            Text {
                text: -limit + index * 2 * limit / (numTicks - 1)
                color: axisColor
                anchors.top: parent.bottom
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }
        }
    }
    Rectangle {
        id: bar
        height: tickHeight / 2
        color: barColor
        anchors.verticalCenter: axis.verticalCenter
    }
    onValueChanged: {
        bar.width = Math.abs(value) * axis.width / (2 * limit)
        if (value >= 0) {
            bar.x = axis.width / 2 + 1
        } else {
            bar.x = axis.width / 2 - bar.width
        }
    }
}
