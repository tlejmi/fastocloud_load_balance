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

#include <fastotv/commands_info/notification_text_info.h>

#include "base/front_subscriber_info.h"
#include "base/server_auth_info.h"

namespace fastocloud {
namespace server {
namespace base {

class SubscriberInfo {
 public:
  typedef common::Optional<ServerDBAuthInfo> login_t;

  SubscriberInfo();

  void SetCurrentStreamID(fastotv::stream_id_t sid);
  fastotv::stream_id_t GetCurrentStreamID() const;

  void SetLogin(login_t login);
  login_t GetLogin() const;

  virtual common::Optional<FrontSubscriberInfo> MakeFrontSubscriberInfo() const = 0;
  virtual common::ErrnoError SendNotification(const fastotv::commands_info::NotificationTextInfo& notify) = 0;

 private:
  login_t login_;
  fastotv::stream_id_t current_stream_id_;
};

}  // namespace base
}  // namespace server
}  // namespace fastocloud
