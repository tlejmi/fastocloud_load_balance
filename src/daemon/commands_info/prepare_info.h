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

#include <common/file_system/path.h>
#include <common/net/net.h>
#include <common/serializer/json_serializer.h>

namespace fastocloud {
namespace server {
namespace service {

class PrepareInfo : public common::serializer::JsonSerializer<PrepareInfo> {
 public:
  typedef JsonSerializer<PrepareInfo> base_class;

  PrepareInfo();
  PrepareInfo(const common::net::HostAndPort& host,
              const common::file_system::ascii_directory_string_path& catchups_http_root);

  common::net::HostAndPort GetCatchupsHost() const;
  common::file_system::ascii_directory_string_path GetCatchupsHttpRoot() const;

  bool Equals(const PrepareInfo& info) const;

 protected:
  common::Error DoDeSerialize(json_object* serialized) override;
  common::Error SerializeFields(json_object* out) const override;

 private:
  common::net::HostAndPort catchups_host;
  common::file_system::ascii_directory_string_path catchups_http_root;
};

inline bool operator==(const PrepareInfo& left, const PrepareInfo& right) {
  return left.Equals(right);
}

inline bool operator!=(const PrepareInfo& x, const PrepareInfo& y) {
  return !(x == y);
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
