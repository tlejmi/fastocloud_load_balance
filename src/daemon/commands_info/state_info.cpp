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

#include "daemon/commands_info/state_info.h"

#define CLIENTS_FIELD "online_clients"

namespace fastocloud {
namespace server {
namespace service {

StateInfo::StateInfo() : base_class(), clients_() {}

StateInfo::StateInfo(const online_clients_t& clients) : clients_(clients) {}

void StateInfo::SetOnlineClients(const online_clients_t& clients) {
  clients_ = clients;
}

StateInfo::online_clients_t StateInfo::GetOnlineClients() const {
  return clients_;
}

common::Error StateInfo::SerializeFields(json_object* deserialized) const {
  json_object* jclients = json_object_new_array();
  for (auto client : clients_) {
    json_object* jclient = nullptr;
    common::Error err = client.Serialize(&jclient);
    if (err) {
      continue;
    }
    json_object_array_add(jclients, jclient);
  }
  ignore_result(SetArrayField(deserialized, CLIENTS_FIELD, jclients));
  return common::Error();
}

common::Error StateInfo::DoDeSerialize(json_object* serialized) {
  UNUSED(serialized);

  size_t len;
  json_object* jclients;
  common::Error err = GetArrayField(serialized, CLIENTS_FIELD, &jclients, &len);
  online_clients_t clients;
  if (!err) {
    for (size_t i = 0; i < len; ++i) {
      json_object* jclient = json_object_array_get_idx(jclients, i);
      base::FrontSubscriberInfo client;
      common::Error err = client.DeSerialize(jclient);
      if (err) {
        continue;
      }
      clients.push_back(client);
    }
  }

  *this = StateInfo(clients);
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
