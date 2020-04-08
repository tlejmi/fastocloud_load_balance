/*  Copyright (C) 2014-2020 FastoGT. All right reserved.
    This file is part of fastocloud.
    fastocloud is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    fastocloud is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with fastocloud.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include <fastotv/commands_info/catchups_info.h>
#include <fastotv/protocol/types.h>

#include "base/front_subscriber_info.h"

// daemon
// client commands

#define DAEMON_ACTIVATE "activate_request"  // {"key": "XXXXXXXXXXXXXXXXXX"}
#define DAEMON_STOP_SERVICE "stop_service"  // {"delay": 0 }
#define DAEMON_PING_SERVICE "ping_service"
#define DAEMON_PREPARE_SERVICE "prepare_service"
#define DAEMON_SYNC_SERVICE "sync_service"
#define DAEMON_GET_LOG_SERVICE "get_log_service"

// subscriber
#define DAEMON_SERVER_PING "ping_client"
#define DAEMON_CLIENT_SEND_MESSAGE "send_message"

// Broadcast
#define DAEMON_SERVER_CATCHUP_CREATED "catchup_created"
#define STREAM_STATISTIC_SERVICE "statistic_service"
#define DAEMON_SERVER_SUBSCRIBER_CONNECTED "subscriber_connected"
#define DAEMON_SERVER_SUBSCRIBER_DISCONNECTED "subscriber_disconnected"

namespace fastocloud {
namespace server {

common::Error CatchupCreatedBroadcast(const fastotv::commands_info::CatchupInfo& params,
                                      fastotv::protocol::request_t* req);

common::Error StatisitcServiceBroadcast(fastotv::protocol::serializet_params_t params,
                                        fastotv::protocol::request_t* req);

common::Error SubscriberConnectedBroadcast(const base::FrontSubscriberInfo& subs, fastotv::protocol::request_t* req);
common::Error SubscriberDisConnectedBroadcast(const base::FrontSubscriberInfo& subs, fastotv::protocol::request_t* req);

}  // namespace server
}  // namespace fastocloud
