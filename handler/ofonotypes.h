#pragma once

#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QMetaType>

struct OfonoModem {
    QDBusObjectPath path;
    QVariantMap properties;
};

Q_DECLARE_METATYPE(OfonoModem)

typedef QList<OfonoModem> OfonoModemList;

Q_DECLARE_METATYPE(OfonoModemList)

inline QDBusArgument &operator<<(QDBusArgument &arg, const OfonoModem &modem)
{
    arg.beginStructure();
    arg << modem.path << modem.properties;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, OfonoModem &modem)
{
    arg.beginStructure();
    arg >> modem.path >> modem.properties;
    arg.endStructure();
    return arg;
}
