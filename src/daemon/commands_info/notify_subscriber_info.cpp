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

  json_object_object_add(deserialized, UID_FIELD, json_object_new_string(uid_.c_str()));
  json_object_object_add(deserialized, DID_FIELD, json_object_new_string(device_id_.c_str()));
  return base_class::SerializeFields(deserialized);
}

common::Error NotifySubscriberInfo::DoDeSerialize(json_object* serialized) {
  NotifySubscriberInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, UID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  inf.uid_ = json_object_get_string(jid);

  json_object* jdid = nullptr;
  json_bool jdid_exists = json_object_object_get_ex(serialized, DID_FIELD, &jdid);
  if (!jdid_exists) {
    return common::make_error_inval();
  }

  inf.device_id_ = json_object_get_string(jdid);

  *this = inf;
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
