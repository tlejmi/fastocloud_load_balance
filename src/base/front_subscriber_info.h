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

#include <common/serializer/json_serializer.h>

#include <fastotv/commands_info/client_info.h>

namespace fastocloud {
namespace server {
namespace base {

class FrontSubscriberInfo : public common::serializer::JsonSerializer<FrontSubscriberInfo> {
 public:
  typedef common::Optional<fastotv::commands_info::ClientInfo> client_info_t;

  FrontSubscriberInfo();
  FrontSubscriberInfo(const fastotv::user_id_t& uid,
                      const fastotv::login_t& login,
                      const fastotv::device_id_t& device_id,
                      fastotv::timestamp_t exp_date,
                      client_info_t client_info = client_info_t());

  bool IsValid() const;

  fastotv::login_t GetLogin() const;
  void SetLogin(const fastotv::login_t& login);

  fastotv::user_id_t GetUserID() const;
  void SetUserID(const fastotv::user_id_t& uid);

  fastotv::device_id_t GetDeviceID() const;
  void SetDeviceID(fastotv::device_id_t dev);

  fastotv::timestamp_t GetExpiredDate() const;
  void SetExpiredDate(fastotv::timestamp_t date);

  void SetClientInfo(const client_info_t& info);
  client_info_t GetClientInfo() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  fastotv::user_id_t uid_;
  fastotv::login_t login_;
  fastotv::device_id_t device_id_;
  fastotv::timestamp_t expired_date_;
  client_info_t clinet_info_;
};

}  // namespace base
}  // namespace server
}  // namespace fastocloud
