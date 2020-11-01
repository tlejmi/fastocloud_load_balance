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

#include "daemon/commands_info/notify_subscriber_info.h"

#define UID_FIELD "id"
#define DID_FIELD "device_id"

namespace fastocloud {
namespace server {
namespace service {

NotifySubscriberInfo::NotifySubscriberInfo() : base_class() {}

NotifySubscriberInfo::NotifySubscriberInfo(fastotv::user_id_t uid, fastotv::device_id_t did)
    : base_class(), uid_(uid), device_id_(did) {}

bool NotifySubscriberInfo::IsValid() const {
  return !uid_.empty() && !device_id_.empty();
}

fastotv::user_id_t NotifySubscriberInfo::GetUserID() const {
  return uid_;
}

void NotifySubscriberInfo::SetUserID(fastotv::user_id_t uid) {
  uid_ = uid;
}

fastotv::device_id_t NotifySubscriberInfo::GetDeviceID() const {
  return device_id_;
}

void NotifySubscriberInfo::SetDeviceID(fastotv::device_id_t dev) {
  device_id_ = dev;
}

common::Error NotifySubscriberInfo::SerializeFields(json_object* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  ignore_result(SetStringField(deserialized, UID_FIELD, uid_));
  ignore_result(SetStringField(deserialized, DID_FIELD, device_id_));
  return base_class::SerializeFields(deserialized);
}

common::Error NotifySubscriberInfo::DoDeSerialize(json_object* serialized) {
  NotifySubscriberInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  fastotv::user_id_t uid;
  err = GetStringField(serialized, UID_FIELD, &uid);
  if (err) {
    return err;
  }

  fastotv::device_id_t did;
  err = GetStringField(serialized, DID_FIELD, &did);
  if (err) {
    return err;
  }

  *this = NotifySubscriberInfo(uid, did);
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
