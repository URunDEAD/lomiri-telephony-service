include(FindGettext)

set(qt_SRCS
    authhandler.cpp
    callchannelobserver.cpp
    displaynamesettings.cpp
    messagingmenu.cpp
    metrics.cpp
    textchannelobserver.cpp
    ussdindicator.cpp
    notificationmenu.cpp
    voicemailindicator.cpp
    indicatordbus.cpp
    )

set_source_files_properties(
        "${DATA_DIR}/org.freedesktop.Notifications.xml"
        PROPERTIES
        INCLUDE "${CMAKE_SOURCE_DIR}/indicator/DBusTypes.h"
)

set(notifications-interface_SRCS "")
qt5_add_dbus_interface(
        notifications-interface_SRCS
        "${DATA_DIR}/org.freedesktop.Notifications.xml"
        NotificationsInterface
)
add_library(notifications-interface STATIC ${notifications-interface_SRCS})
target_link_libraries(notifications-interface PUBLIC Qt5::DBus)

qt5_add_dbus_adaptor(qt_SRCS Indicator.xml indicator/indicatordbus.h IndicatorDBus)

set(indicator_SRCS main.cpp ${qt_SRCS})


include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${NOTIFY_INCLUDE_DIRS}
    ${MESSAGING_MENU_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/liblomiritelephonyservice
    ${CMAKE_CURRENT_BINARY_DIR}
    ${UserMetrics_INCLUDE_DIRS}
    ${HISTORY_INCLUDE_DIRS}
    )

link_directories(${MESSAGING_MENU_LIBRARY_DIRS})

add_executable(lomiri-indicator-telephony-service ${indicator_SRCS} ${indicator_HDRS})

target_link_libraries(lomiri-indicator-telephony-service
    ${TP_QT5_LIBRARIES}
    ${NOTIFY_LIBRARIES}
    ${MESSAGING_MENU_LIBRARIES}
    ${UserMetrics_LIBRARIES}
    ${HISTORY_LIBRARIES}
    lomiritelephonyservice
    notifications-interface
    Qt5::Contacts
    Qt5::Core
    Qt5::DBus
    Qt5::Gui
    Qt5::Multimedia
    Qt5::Qml
    )

enable_coverage(lomiri-indicator-telephony-service)

configure_file(org.freedesktop.Telepathy.Client.TelephonyServiceIndicator.service.in org.freedesktop.Telepathy.Client.TelephonyServiceIndicator.service)
install(TARGETS lomiri-indicator-telephony-service RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.TelephonyServiceIndicator.service DESTINATION share/dbus-1/services)
install(FILES LomiriTelephonyServiceIndicator.client DESTINATION share/telepathy/clients)

# Handle i18n in desktop files
set(desktop_FILES lomiri-telephony-service-sms.desktop lomiri-telephony-service-call.desktop)

function(merge_desktop_translations DESKTOP_FILES)
    foreach(DESKTOP_FILE ${DESKTOP_FILES})
        add_custom_target(${DESKTOP_FILE} ALL
            COMMENT "Merging translations into ${DESKTOP_FILE}..."
            COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
                    -d ${CMAKE_SOURCE_DIR}/po
                    --desktop
                    --template=${CMAKE_CURRENT_SOURCE_DIR}/${DESKTOP_FILE}.in
                    -o ${DESKTOP_FILE}
        )

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${DESKTOP_FILE}
                DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    endforeach()
endfunction()

merge_desktop_translations("${desktop_FILES}")

add_subdirectory(icons)
