if (PULSEAUDIO_FOUND)
    add_definitions(-DUSE_PULSEAUDIO)
    set(USE_PULSEAUDIO ON)
    set(QPULSEAUDIOENGINE_CPP qpulseaudioengine.cpp)
endif (PULSEAUDIO_FOUND)

set(qt_SRCS
    accountproperties.cpp
    callagent.cpp
    audioroutemanager.cpp
    callhandler.cpp
    chatstartingjob.cpp
    farstreamchannel.cpp
    handler.cpp
    handlerdbus.cpp
    messagejob.cpp
    messagesendingjob.cpp
    ofonovoicecallagent.cpp
    powerdaudiomodemediator.cpp
    powerddbus.cpp
    texthandler.cpp
    ${QPULSEAUDIOENGINE_CPP}
    )

set(handler_SRCS main.cpp ${qt_SRCS})
qt5_add_dbus_adaptor(handler_SRCS Handler.xml handler/handlerdbus.h HandlerDBus)
qt5_add_dbus_adaptor(handler_SRCS ChatStartingJob.xml handler/chatstartingjob.h ChatStartingJob)
qt5_add_dbus_adaptor(handler_SRCS MessageSendingJob.xml handler/messagesendingjob.h MessageSendingJob)
qt5_add_dbus_adaptor(handler_SRCS org.ofono.VoiceCallAgent.xml handler/ofonovoicecallagent.h OfonoVoiceCallAgent ofonovoicecallagentadaptor OfonoVoiceCallAgentAdaptor)

set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/org.ofono.Manager.xml"
    PROPERTIES INCLUDE "${CMAKE_SOURCE_DIR}/handler/ofonotypes.h"
)
qt5_add_dbus_interface(handler_SRCS org.ofono.Manager.xml OfonoManagerInterface)
qt5_add_dbus_interface(handler_SRCS org.ofono.VoiceCallManager.xml OfonoVoiceCallManagerInterface)

include_directories(
    ${TP_QT5_INCLUDE_DIRS}
    ${TPFS_INCLUDE_DIRS}
    ${FS_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/liblomiritelephonyservice
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PULSEAUDIO_INCLUDE_DIRS}
    ${HISTORY_INCLUDE_DIRS}
    )

add_executable(lomiri-telephony-service-handler ${handler_SRCS} ${handler_HDRS})

target_link_libraries(lomiri-telephony-service-handler
    ${TP_QT5_LIBRARIES}
    ${TP_QT5_FS_LIBRARIES}
    ${TPFS_LIBRARIES}
    ${FS_LIBRARIES}
    ${HISTORY_LIBRARIES}
    lomiritelephonyservice
    ${PULSEAUDIO_LIBRARIES}
    Qt5::Contacts
    Qt5::Core
    Qt5::DBus
    Qt5::Qml
    )

enable_coverage(lomiri-telephony-service-handler)

configure_file(com.lomiri.TelephonyServiceHandler.service.in com.lomiri.TelephonyServiceHandler.service)
configure_file(org.freedesktop.Telepathy.Client.TelephonyServiceHandler.service.in org.freedesktop.Telepathy.Client.TelephonyServiceHandler.service)

install(TARGETS lomiri-telephony-service-handler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.TelephonyServiceHandler.service DESTINATION share/dbus-1/services)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/com.lomiri.TelephonyServiceHandler.service DESTINATION share/dbus-1/services)
install(FILES LomiriTelephonyServiceHandler.client DESTINATION share/telepathy/clients)
