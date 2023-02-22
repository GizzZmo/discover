/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */


import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import org.kde.discover 2.0
import org.kde.kirigami 2.19 as Kirigami

ListView {
    id: root
    readonly property alias count: screenshotsModel.count
    property bool showNavigationArrows: true
    property alias resource: screenshotsModel.application
    property var resource
    property int failedCount: 0
    readonly property bool hasFailed: count !== 0 && failedCount === count

    spacing: Kirigami.Units.largeSpacing
    focus: overlay.visible
    orientation: Qt.Horizontal
    cacheBuffer: 10 // keep some screenshots in memory

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()

    model: ScreenshotsModel {
        id: screenshotsModel
    }

    property real delegateHeight: Kirigami.Units.gridUnit * 4

    delegate: AbstractButton {
        readonly property bool animated: isAnimated
        readonly property url imageSource: large_image_url
        readonly property real proportion: thumbnail.status === Image.Ready && thumbnail.sourceSize.width>1 ? thumbnail.sourceSize.height/thumbnail.sourceSize.width : 1

        implicitWidth: root.delegateHeight / proportion
        implicitHeight: root.delegateHeight
        opacity: hovered ? 0.7 : 1

        hoverEnabled: true
        onClicked: {
            root.currentIndex = model.row
            overlay.open()
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        background: Item {
            BusyIndicator {
                visible: running
                running: thumbnail.status === Image.Loading
                anchors.centerIn: parent
            }
            Kirigami.Icon {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit
                visible: thumbnail.status === Image.Error
                source: "emblem-error"
            }
            ConditionalLoader {
                id: thumbnail
                anchors.fill: parent
                readonly property var status: item.status
                readonly property var sourceSize: item.sourceSize
                condition: isAnimated

                componentFalse: Component {
                    Image {
                        source: small_image_url
                    }
                }
                componentTrue: Component {
                    AnimatedImage {
                        source: small_image_url
                    }
                }

                onStatusChanged: {
                    if (status === Image.Error) {
                        root.failedCount += 1;
                    }
                }
            }
        }
    }

    Popup {
        id: overlay
        parent: applicationWindow().overlay
        z: applicationWindow().globalDrawer.z + 10
        modal: true
        clip: false

        x: (parent.width - width)/2
        y: (parent.height - height)/2
        readonly property real proportion: overlayImage.sourceSize.width>1 ? overlayImage.sourceSize.height/overlayImage.sourceSize.width : 1
        height: overlayImage.status >= Image.Loading ? Kirigami.Units.gridUnit * 5 : Math.min(parent.height * 0.9, (parent.width * 0.9) * proportion, overlayImage.sourceSize.height)
        width: (height - 2 * padding)/proportion

        BusyIndicator {
            id: indicator
            visible: running
            running: overlayImage.status === Image.Loading
            anchors.centerIn: parent
        }

        Kirigami.Icon {
            anchors.fill: parent
            visible: overlayImage.status === Image.Error
            source: "emblem-error"
        }

        ConditionalLoader {
            id: overlayImage
            anchors.fill: parent
            readonly property var status: item.status
            readonly property var sourceSize: item.sourceSize
            condition: root.currentItem.animated

            componentFalse: Component {
                Image {
                    source: root.currentItem ? root.currentItem.imageSource : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }
            componentTrue: Component {
                AnimatedImage {
                    source: root.currentItem ? root.currentItem.imageSource : ""
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            onStatusChanged: {
                if (status === Image.Error) {
                    root.failedCount += 1;
                }
            }
        }

        Button {
            anchors {
                left: parent.right
                bottom: parent.top
            }
            icon.name: "window-close"
            onClicked: overlay.close()
        }


        RoundButton {
            anchors {
                right: parent.left
                verticalCenter: parent.verticalCenter
            }
            visible: leftAction.visible
            icon.name: leftAction.iconName
            onClicked: leftAction.triggered(null)
        }

        RoundButton {
            anchors {
                left: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: rightAction.visible
            icon.name: rightAction.iconName
            onClicked: rightAction.triggered(null)
        }

        Kirigami.Action {
            id: leftAction
            icon.name: root.LayoutMirroring.enabled ? "arrow-right" : "arrow-left"
            enabled: overlay.visible && visible
            visible: root.currentIndex >= 1 && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex - 1) % screenshotsModel.count
        }

        Kirigami.Action {
            id: rightAction
            icon.name: root.LayoutMirroring.enabled ? "arrow-left" : "arrow-right"
            enabled: overlay.visible && visible
            visible: root.currentIndex < (root.count - 1) && !indicator.running
            onTriggered: root.currentIndex = (root.currentIndex + 1) % screenshotsModel.count
        }
    }


    clip: true

    RoundButton {
        anchors {
            left: parent.left
            leftMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        width: Kirigami.Units.gridUnit * 2
        height: width
        icon.name: root.LayoutMirroring.enabled ? "arrow-right" : "arrow-left"
        visible: !Kirigami.Settings.isMobile
                 && root.count > 1
                 && root.currentIndex > 0
                 && root.showNavigationArrows
        Keys.forwardTo: [root]
        onClicked: root.currentIndex -= 1
    }

    RoundButton {
        anchors {
            right: parent.right
            rightMargin: Kirigami.Units.largeSpacing
            verticalCenter: parent.verticalCenter
        }
        width: Kirigami.Units.gridUnit * 2
        height: width
        icon.name: root.LayoutMirroring.enabled ? "arrow-left" : "arrow-right"
        visible: !Kirigami.Settings.isMobile
                 && root.count > 1
                 && root.currentIndex < root.count - 1
                 && root.showNavigationArrows
        Keys.forwardTo: [root]
        onClicked: root.currentIndex += 1
    }
}
