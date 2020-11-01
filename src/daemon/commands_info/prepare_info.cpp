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

#define CATCHUPS_HOST_FIELD "catchups_host"
#define CATCHUPS_HTTP_ROOT_FIELD "catchups_http_root"

namespace fastocloud {
namespace server {
namespace service {

PrepareInfo::PrepareInfo() : base_class(), catchups_host(), catchups_http_root() {}

PrepareInfo::PrepareInfo(const common::net::HostAndPort& host,
                         const common::file_system::ascii_directory_string_path& catchups_http_root)
    : base_class(), catchups_host(host), catchups_http_root(catchups_http_root) {}

common::net::HostAndPort PrepareInfo::GetCatchupsHost() const {
  return catchups_host;
}

common::file_system::ascii_directory_string_path PrepareInfo::GetCatchupsHttpRoot() const {
  return catchups_http_root;
}

bool PrepareInfo::Equals(const PrepareInfo& info) const {
  return catchups_host == info.catchups_host && catchups_http_root == info.catchups_http_root;
}

common::Error PrepareInfo::SerializeFields(json_object* out) const {
  std::string catchups_host_str = common::ConvertToString(catchups_host);
  ignore_result(SetStringField(out, CATCHUPS_HOST_FIELD, catchups_host_str));
  std::string catchups_http_root_str = catchups_http_root.GetPath();
  ignore_result(SetStringField(out, CATCHUPS_HTTP_ROOT_FIELD, catchups_http_root_str));
  return common::Error();
}

common::Error PrepareInfo::DoDeSerialize(json_object* serialized) {
  std::string catchups_host;
  common::net::HostAndPort host;
  ignore_result(GetStringField(serialized, CATCHUPS_HOST_FIELD, &catchups_host));
  ignore_result(common::ConvertFromString(catchups_host, &host));

  std::string http_root;
  ignore_result(GetStringField(serialized, CATCHUPS_HTTP_ROOT_FIELD, &http_root));

  *this = PrepareInfo(host, common::file_system::ascii_directory_string_path(http_root));
  return common::Error();
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
