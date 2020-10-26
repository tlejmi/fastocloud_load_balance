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
#include <vector>

#include <common/serializer/json_serializer.h>

#include "base/front_subscriber_info.h"

namespace fastocloud {
namespace server {
namespace service {

class StateInfo : public common::serializer::JsonSerializer<StateInfo> {
 public:
  typedef JsonSerializer<StateInfo> base_class;
  typedef std::vector<base::FrontSubscriberInfo> online_clients_t;

  StateInfo();
  StateInfo(const online_clients_t& clients);

  void SetOnlineClients(const online_clients_t& clients);
  online_clients_t GetOnlineClients() const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* deserialized) const override;

 private:
  online_clients_t clients_;
};

}  // namespace service
}  // namespace server
}  // namespace fastocloud
