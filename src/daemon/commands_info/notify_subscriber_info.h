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

#include <fastotv/types.h>

#include <fastotv/commands_info/notification_text_info.h>

namespace fastocloud {
namespace server {
namespace service {

class NotifySubscriberInfo : public fastotv::commands_info::NotificationTextInfo {
 public:
  typedef fastotv::commands_info::NotificationTextInfo base_class;

  NotifySubscriberInfo();
  NotifySubscriberInfo(fastotv::user_id_t uid, fastotv::device_id_t did);

  bool IsValid() const;

  fastotv::user_id_t GetUserID() const;
  void SetUserID(fastotv::user_id_t uid);

  fastotv::device_id_t GetDeviceID() const;
  void SetDeviceID(fastotv::device_id_t dev);

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  fastotv::user_id_t uid_;
  fastotv::device_id_t device_id_;
};

}  // namespace service
}  // namespace server
}  // namespace fastocloud
