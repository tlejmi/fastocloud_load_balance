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

#include <string>

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
  std::string sid;
  common::Error err = GetStringField(serialized, UID_FIELD, &sid);
  if (err) {
    return err;
  }

  std::string login;
  err = GetStringField(serialized, LOGIN_FIELD, &login);
  if (err) {
    return err;
  }

  std::string did;
  err = GetStringField(serialized, DEVICE_ID_FIELD, &did);
  if (err) {
    return err;
  }

  int64_t exp;
  err = GetInt64Field(serialized, EXPIRED_DATE_FIELD, &exp);
  if (err) {
    return err;
  }

  *this = FrontSubscriberInfo(sid, login, did, exp);
  return common::Error();
}

common::Error FrontSubscriberInfo::SerializeFields(json_object* deserialized) const {
  if (!IsValid()) {
    return common::make_error_inval();
  }

  ignore_result(SetStringField(deserialized, UID_FIELD, uid_));
  ignore_result(SetStringField(deserialized, LOGIN_FIELD, login_));
  ignore_result(SetStringField(deserialized, DEVICE_ID_FIELD, device_id_));
  ignore_result(SetInt64Field(deserialized, EXPIRED_DATE_FIELD, expired_date_));
  return common::Error();
}

}  // namespace base
}  // namespace server
}  // namespace fastocloud
