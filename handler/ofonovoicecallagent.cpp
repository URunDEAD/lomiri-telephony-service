/*
 * Copyright (C) 2026 Marius Gripsgard <marius@ubports.com>
 *
 * This file is part of lomiri-telephony-service.
 *
 * lomiri-telephony-service is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * lomiri-telephony-service is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ofonovoicecallagent.h"
#include "ofonovoicecallagentadaptor.h"
#include "OfonoManagerInterface.h"
#include "OfonoVoiceCallManagerInterface.h"
#include "ofonotypes.h"
#include "tonegenerator.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusObjectPath>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDebug>

#define OFONO_SERVICE       "org.ofono"
#define OFONO_MANAGER_PATH  "/"
#define OFONO_VCM_INTERFACE "org.ofono.VoiceCallManager"

OfonoVoiceCallAgent::OfonoVoiceCallAgent(QObject *parent)
    : QObject(parent),
      mManagerProxy(nullptr),
      mOfonoWatcher(nullptr)
{
    qDBusRegisterMetaType<OfonoModem>();
    qDBusRegisterMetaType<OfonoModemList>();

    new OfonoVoiceCallAgentAdaptor(this);

    if (!QDBusConnection::systemBus().registerObject(OFONO_AGENT_OBJECT_PATH, this)) {
        qWarning() << "OfonoVoiceCallAgent: failed to register object on system bus";
        return;
    }

    mOfonoWatcher = new QDBusServiceWatcher(
        OFONO_SERVICE,
        QDBusConnection::systemBus(),
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this);

    connect(mOfonoWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &OfonoVoiceCallAgent::onOfonoServiceRegistered);
    connect(mOfonoWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &OfonoVoiceCallAgent::onOfonoServiceUnregistered);

    if (QDBusConnection::systemBus().interface()->isServiceRegistered(OFONO_SERVICE)) {
        connectManagerSignals();
        registerModems();
    }
}

OfonoVoiceCallAgent::~OfonoVoiceCallAgent()
{
    unregisterFromOfono();
    cleanupState();
    disconnectManagerSignals();
    QDBusConnection::systemBus().unregisterObject(OFONO_AGENT_OBJECT_PATH);
}

OfonoVoiceCallAgent *OfonoVoiceCallAgent::instance()
{
    static OfonoVoiceCallAgent *self = new OfonoVoiceCallAgent();
    return self;
}

void OfonoVoiceCallAgent::Release()
{
    qDebug() << "OfonoVoiceCallAgent: Release called by oFono";
    ToneGenerator::instance()->stopRingbackTone();
    // We don't know which modem released us, so clear all and
    // registerModems again.
    mVcmModemPaths.clear();
    registerModems();
}

void OfonoVoiceCallAgent::RingbackTone(bool playTone)
{
    qDebug() << "OfonoVoiceCallAgent: RingbackTone" << playTone;
    if (playTone) {
        ToneGenerator::instance()->playRingbackTone();
    } else {
        ToneGenerator::instance()->stopRingbackTone();
    }
}

void OfonoVoiceCallAgent::onOfonoServiceRegistered(const QString &service)
{
    Q_UNUSED(service);
    qDebug() << "OfonoVoiceCallAgent: oFono appeared on D-Bus";
    connectManagerSignals();
    registerModems();
}

void OfonoVoiceCallAgent::onOfonoServiceUnregistered(const QString &service)
{
    Q_UNUSED(service);
    qDebug() << "OfonoVoiceCallAgent: oFono disappeared from D-Bus";
    disconnectManagerSignals();
    cleanupState();
}

void OfonoVoiceCallAgent::onModemAdded(const QDBusObjectPath &path, const QVariantMap &properties)
{
    const QStringList interfaces = qdbus_cast<QStringList>(properties.value("Interfaces"));
    if (!interfaces.contains(OFONO_VCM_INTERFACE)) {
        return;
    }
    registerModem(path.path());
}

void OfonoVoiceCallAgent::onModemRemoved(const QDBusObjectPath &path)
{
    const QString modemPath = path.path();
    if (mVcmModemPaths.removeAll(modemPath) > 0) {
        qDebug() << "OfonoVoiceCallAgent: modem removed:" << modemPath;
    }
}

void OfonoVoiceCallAgent::registerModems()
{
    if (!mManagerProxy) {
        return;
    }

    QDBusPendingReply<OfonoModemList> pending = mManagerProxy->GetModems();
    pending.waitForFinished();
    if (pending.isError()) {
        qWarning() << "OfonoVoiceCallAgent: GetModems failed:" << pending.error().message();
        return;
    }

    for (const OfonoModem &modem : pending.value()) {
        const QStringList interfaces = qdbus_cast<QStringList>(modem.properties.value("Interfaces"));
        if (interfaces.contains(OFONO_VCM_INTERFACE))
            registerModem(modem.path.path());
    }
}

bool OfonoVoiceCallAgent::registerModem(const QString &modemPath)
{
    if (mVcmModemPaths.contains(modemPath)) {
        return true;
    }

    OrgOfonoVoiceCallManagerInterface vcm(OFONO_SERVICE, modemPath,
                                          QDBusConnection::systemBus());
    QDBusPendingReply<> pending = vcm.RegisterVoicecallAgent(
        QDBusObjectPath(OFONO_AGENT_OBJECT_PATH));
    pending.waitForFinished();
    if (pending.isError()) {
        qWarning() << "OfonoVoiceCallAgent: RegisterVoicecallAgent failed on"
                   << modemPath << ":" << pending.error().message();
        return false;
    }

    mVcmModemPaths << modemPath;
    qDebug() << "OfonoVoiceCallAgent: registered on modem" << modemPath;
    return true;
}

void OfonoVoiceCallAgent::unregisterFromOfono()
{
    for (const QString &path : mVcmModemPaths) {
        OrgOfonoVoiceCallManagerInterface vcm(OFONO_SERVICE, path,
                                              QDBusConnection::systemBus());
        vcm.UnregisterVoicecallAgent(QDBusObjectPath(OFONO_AGENT_OBJECT_PATH));
    }
    mVcmModemPaths.clear();
}

void OfonoVoiceCallAgent::cleanupState()
{
    ToneGenerator::instance()->stopRingbackTone();
    mVcmModemPaths.clear();
}

void OfonoVoiceCallAgent::connectManagerSignals()
{
    mManagerProxy = new OrgOfonoManagerInterface(
        OFONO_SERVICE, OFONO_MANAGER_PATH,
        QDBusConnection::systemBus(), this);

    connect(mManagerProxy, &OrgOfonoManagerInterface::ModemAdded,
            this, &OfonoVoiceCallAgent::onModemAdded);
    connect(mManagerProxy, &OrgOfonoManagerInterface::ModemRemoved,
            this, &OfonoVoiceCallAgent::onModemRemoved);
}

void OfonoVoiceCallAgent::disconnectManagerSignals()
{
    if (mManagerProxy) {
        mManagerProxy->deleteLater();
        mManagerProxy = nullptr;
    }
}
