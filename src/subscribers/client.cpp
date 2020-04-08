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

#include "subscribers/client.h"

#define NOTIFY_MESSAGE "send_message"

namespace fastotv {
namespace {
common::Error NotifyRequest(protocol::sequance_id_t id,
                            const commands_info::NotificationTextInfo& params,
                            protocol::request_t* req) {
  if (!req) {
    return common::make_error_inval();
  }

  std::string ping_client_json;
  common::Error err_ser = params.SerializeToString(&ping_client_json);
  if (err_ser) {
    return err_ser;
  }

  protocol::request_t lreq;
  lreq.id = id;
  lreq.method = NOTIFY_MESSAGE;
  lreq.params = ping_client_json;
  *req = lreq;
  return common::Error();
}
}  // namespace
}  // namespace fastotv

namespace fastocloud {
namespace server {
namespace subscribers {

SubscriberClient::SubscriberClient(common::libev::IoLoop* server, const common::net::socket_info& info)
    : base_class(server, info), client_info_() {}

const char* SubscriberClient::ClassName() const {
  return "SubscriberClient";
}

void SubscriberClient::SetClInfo(const client_info_t& info) {
  client_info_ = info;
}

SubscriberClient::client_info_t SubscriberClient::GetClInfo() const {
  return client_info_;
}

common::Optional<base::FrontSubscriberInfo> SubscriberClient::MakeFrontSubscriberInfo() const {
  const auto login = GetLogin();
  if (!login) {
    return common::Optional<base::FrontSubscriberInfo>();
  }

  return base::FrontSubscriberInfo(login->GetUserID(), login->GetLogin(), login->GetDeviceID(),
                                   login->GetExpiredDate());
}

common::ErrnoError SubscriberClient::SendNotification(const fastotv::commands_info::NotificationTextInfo& notify) {
  fastotv::protocol::request_t notify_request;
  common::Error err_ser = fastotv::NotifyRequest(NextRequestID(), notify, &notify_request);
  if (err_ser) {
    return common::make_errno_error(err_ser->GetDescription(), EAGAIN);
  }

  return WriteRequest(notify_request);
}

}  // namespace subscribers
}  // namespace server
}  // namespace fastocloud
