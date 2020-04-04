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

#include "daemon/commands.h"

namespace fastocloud {
namespace server {

common::Error CatchupCreatedBroadcast(const fastotv::commands_info::CatchupInfo& params,
                                      fastotv::protocol::request_t* req) {
  if (!req) {
    return common::make_error_inval();
  }

  std::string catchup;
  common::Error err_ser = params.SerializeToString(&catchup);
  if (err_ser) {
    return err_ser;
  }

  *req = fastotv::protocol::request_t::MakeNotification(DAEMON_SERVER_CATCHUP_CREATED, catchup);
  return common::Error();
}

common::Error StatisitcServiceBroadcast(fastotv::protocol::serializet_params_t params,
                                        fastotv::protocol::request_t* req) {
  if (!req) {
    return common::make_error_inval();
  }

  *req = fastotv::protocol::request_t::MakeNotification(STREAM_STATISTIC_SERVICE, params);
  return common::Error();
}

common::Error SubscriberConnectedBroadcast(const base::FrontSubscriberInfo& subs, fastotv::protocol::request_t* req) {
  if (!req) {
    return common::make_error_inval();
  }

  std::string subscriber;
  common::Error err_ser = subs.SerializeToString(&subscriber);
  if (err_ser) {
    return err_ser;
  }

  *req = fastotv::protocol::request_t::MakeNotification(DAEMON_SERVER_SUBSCRIBER_CONNECTED, subscriber);
  return common::Error();
}

common::Error SubscriberDisConnectedBroadcast(const base::FrontSubscriberInfo& subs,
                                              fastotv::protocol::request_t* req) {
  if (!req) {
    return common::make_error_inval();
  }

  std::string subscriber;
  common::Error err_ser = subs.SerializeToString(&subscriber);
  if (err_ser) {
    return err_ser;
  }

  *req = fastotv::protocol::request_t::MakeNotification(DAEMON_SERVER_SUBSCRIBER_DISCONNECTED, subscriber);
  return common::Error();
}

}  // namespace server
}  // namespace fastocloud
