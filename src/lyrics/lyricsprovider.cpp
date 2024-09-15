/*
 * Strawberry Music Player
 * Copyright 2018-2021, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <QString>

#include "core/shared_ptr.h"
#include "core/networkaccessmanager.h"
#include "lyricsprovider.h"

LyricsProvider::LyricsProvider(const QString &name, const bool enabled, const bool authentication_required, SharedPtr<NetworkAccessManager> network, QObject *parent)
    : QObject(parent), network_(network), name_(name), enabled_(enabled), order_(0), authentication_required_(authentication_required) {}

bool LyricsProvider::StartSearchAsync(const int id, const LyricsSearchRequest &request) {

  QMetaObject::invokeMethod(this, "StartSearch", Qt::QueuedConnection, Q_ARG(int, id), Q_ARG(LyricsSearchRequest, request));

  return true;

}
