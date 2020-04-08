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

#include <common/libev/http/http_client.h>

#include "base/subscriber_info.h"

namespace fastocloud {
namespace server {
namespace http {

class HttpClient : public common::libev::http::HttpClient, public base::SubscriberInfo {
 public:
  typedef common::libev::http::HttpClient base_class;

  HttpClient(common::libev::IoLoop* server, const common::net::socket_info& info);

  bool IsVerified() const;
  void SetVerified(bool verified);

  common::Optional<base::FrontSubscriberInfo> MakeFrontSubscriberInfo() const override;

  common::ErrnoError SendNotification(const fastotv::commands_info::NotificationTextInfo& notify) override;

  const char* ClassName() const override;

 private:
  bool is_verified_;
};

}  // namespace http
}  // namespace server
}  // namespace fastocloud
