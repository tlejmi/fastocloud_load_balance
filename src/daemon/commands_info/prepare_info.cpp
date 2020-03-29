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

#include "daemon/commands_info/prepare_info.h"

#define PREPARE_SERVICE_INFO_CATCHUPS_HOST_FIELD "catchups_host"
#define PREPARE_SERVICE_INFO_CATCHUPS_HTTP_ROOT_FIELD "catchups_http_root"

namespace fastocloud {
namespace server {
namespace service {

PrepareInfo::PrepareInfo() : base_class(), catchups_host(), catchups_http_root() {}

common::net::HostAndPort PrepareInfo::GetCatchupsHost() const {
  return catchups_host;
}

common::file_system::ascii_directory_string_path PrepareInfo::GetCatchupsHttpRoot() const {
  return catchups_http_root;
}

common::Error PrepareInfo::SerializeFields(json_object* out) const {
  std::string catchups_host_str = common::ConvertToString(catchups_host);
  json_object_object_add(out, PREPARE_SERVICE_INFO_CATCHUPS_HOST_FIELD,
                         json_object_new_string(catchups_host_str.c_str()));
  std::string catchups_http_root_str = catchups_http_root.GetPath();
  json_object_object_add(out, PREPARE_SERVICE_INFO_CATCHUPS_HTTP_ROOT_FIELD,
                         json_object_new_string(catchups_http_root_str.c_str()));
  return common::Error();
}

common::Error PrepareInfo::DoDeSerialize(json_object* serialized) {
  PrepareInfo inf;
  json_object* jcatchups_host = nullptr;
  json_bool jcatchups_host_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_CATCHUPS_HOST_FIELD, &jcatchups_host);
  if (jcatchups_host_exists) {
    common::net::HostAndPort host;
    if (common::ConvertFromString(json_object_get_string(jcatchups_host), &host)) {
      inf.catchups_host = host;
    }
  }

  json_object* jcatchups_http_root = nullptr;
  json_bool jcatchups_http_root_exists =
      json_object_object_get_ex(serialized, PREPARE_SERVICE_INFO_CATCHUPS_HTTP_ROOT_FIELD, &jcatchups_http_root);
  if (jcatchups_http_root_exists) {
    inf.catchups_http_root =
        common::file_system::ascii_directory_string_path(json_object_get_string(jcatchups_http_root));
  }

  *this = inf;
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
