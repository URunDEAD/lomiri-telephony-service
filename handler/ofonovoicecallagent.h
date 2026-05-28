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

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class OrgOfonoManagerInterface;
class QDBusObjectPath;
class QDBusServiceWatcher;

#define OFONO_AGENT_OBJECT_PATH "/com/lomiri/telephony/VoiceCallAgent"

class OfonoVoiceCallAgent : public QObject
{
    Q_OBJECT

public:
    static OfonoVoiceCallAgent *instance();
    ~OfonoVoiceCallAgent();

public Q_SLOTS:
    // org.ofono.VoiceCallAgent interface
    void Release();
    void RingbackTone(bool playTone);

private Q_SLOTS:
    void onOfonoServiceRegistered(const QString &service);
    void onOfonoServiceUnregistered(const QString &service);
    void onModemAdded(const QDBusObjectPath &path, const QVariantMap &properties);
    void onModemRemoved(const QDBusObjectPath &path);

private:
    explicit OfonoVoiceCallAgent(QObject *parent = nullptr);
    void registerModems();
    bool registerModem(const QString &modemPath);
    void unregisterFromOfono();
    void cleanupState();
    void connectManagerSignals();
    void disconnectManagerSignals();

    OrgOfonoManagerInterface *mManagerProxy;
    QDBusServiceWatcher *mOfonoWatcher;
    QStringList mVcmModemPaths;
};
