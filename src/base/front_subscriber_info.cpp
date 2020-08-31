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

#include "base/front_subscriber_info.h"

#define LOGIN_FIELD "login"
#define UID_FIELD "id"
#define DEVICE_ID_FIELD "device_id"
#define EXPIRED_DATE_FIELD "exp_date"
#define DEVICE_FIELD "device"

namespace fastocloud {
namespace server {
namespace base {

FrontSubscriberInfo::FrontSubscriberInfo() : uid_(), login_(), device_id_(), expired_date_(0) {}

FrontSubscriberInfo::FrontSubscriberInfo(const fastotv::user_id_t& uid,
                                         const fastotv::login_t& login,
                                         const fastotv::device_id_t& device_id,
                                         fastotv::timestamp_t exp_date)
    : uid_(uid), login_(login), device_id_(device_id), expired_date_(exp_date) {}

bool FrontSubscriberInfo::IsValid() const {
  return !login_.empty() && !uid_.empty() && !device_id_.empty() && expired_date_ != 0;
}

fastotv::login_t FrontSubscriberInfo::GetLogin() const {
  return login_;
}

void FrontSubscriberInfo::SetLogin(const fastotv::login_t& login) {
  login_ = login;
}

fastotv::user_id_t FrontSubscriberInfo::GetUserID() const {
  return uid_;
}

void FrontSubscriberInfo::SetUserID(const fastotv::user_id_t& uid) {
  uid_ = uid;
}

fastotv::device_id_t FrontSubscriberInfo::GetDeviceID() const {
  return device_id_;
}

void FrontSubscriberInfo::SetDeviceID(fastotv::device_id_t dev) {
  device_id_ = dev;
}

fastotv::timestamp_t FrontSubscriberInfo::GetExpiredDate() const {
  return expired_date_;
}

void FrontSubscriberInfo::SetExpiredDate(fastotv::timestamp_t date) {
  expired_date_ = date;
}

common::Error FrontSubscriberInfo::DoDeSerialize(json_object* serialized) {
  json_object* jid = nullptr;
  json_bool jid_exists = json_object_object_get_ex(serialized, UID_FIELD, &jid);
  if (!jid_exists) {
    return common::make_error_inval();
  }

  json_object* jlogin = nullptr;
  json_bool jlogin_exists = json_object_object_get_ex(serialized, LOGIN_FIELD, &jlogin);
  if (!jlogin_exists) {
    return common::make_error_inval();
  }

  json_object* jdevid = nullptr;
  json_bool jdevid_exists = json_object_object_get_ex(serialized, DEVICE_ID_FIELD, &jdevid);
  if (!jdevid_exists) {
    return common::make_error_inval();
  }

  json_object* jexp = nullptr;
  json_bool jexp_exists = json_object_object_get_ex(serialized, EXPIRED_DATE_FIELD, &jexp);
  if (!jexp_exists) {
    return common::make_error_inval();
  }

  FrontSubscriberInfo ainf(json_object_get_string(jid), json_object_get_string(jlogin), json_object_get_string(jdevid),
                           json_object_get_int64(jexp));
  *this = ainf;
  return common::Error();
}

common::Error FrontSubscriberInfo::SerializeFields(json_object* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  json_object_object_add(deserialized, UID_FIELD, json_object_new_string(uid_.c_str()));
  json_object_object_add(deserialized, LOGIN_FIELD, json_object_new_string(login_.c_str()));
  json_object_object_add(deserialized, DEVICE_ID_FIELD, json_object_new_string(device_id_.c_str()));
  json_object_object_add(deserialized, EXPIRED_DATE_FIELD, json_object_new_int64(expired_date_));
  return common::Error();
}

}  // namespace base
}  // namespace server
}  // namespace fastocloud
