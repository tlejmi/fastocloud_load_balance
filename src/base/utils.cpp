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

#include "base/utils.h"

#include <common/convert2string.h>
#include <common/net/http_client.h>

namespace {
bool GetHttpHostAndPort(const std::string& host, common::net::HostAndPort* out) {
  if (host.empty() || !out) {
    return false;
  }

  common::net::HostAndPort http_server;
  size_t del = host.find_last_of(':');
  if (del != std::string::npos) {
    http_server.SetHost(host.substr(0, del));
    std::string port_str = host.substr(del + 1);
    uint16_t lport;
    if (common::ConvertFromString(port_str, &lport)) {
      http_server.SetPort(lport);
    }
  } else {
    http_server.SetHost(host);
    http_server.SetPort(80);
  }
  *out = http_server;
  return true;
}

bool GetPostServerFromUrl(const common::uri::Url& url, common::net::HostAndPort* out) {
  if (!url.IsValid() || !out) {
    return false;
  }

  const std::string host_str = url.GetHost();
  return GetHttpHostAndPort(host_str, out);
}
}  // namespace

namespace fastocloud {
namespace base {

common::Error PostHttpFile(const common::file_system::ascii_file_string_path& file_path, const common::uri::Url& url) {
  common::net::HostAndPort http_server_address;
  if (!GetPostServerFromUrl(url, &http_server_address)) {
    return common::make_error_inval();
  }

  common::net::HttpClient cl(http_server_address);
  common::ErrnoError errn = cl.Connect();
  if (errn) {
    return common::make_error_from_errno(errn);
  }

  const auto path = url.GetPath();
  common::Error err = cl.PostFile(path, file_path);
  if (err) {
    cl.Disconnect();
    return err;
  }

  common::http::HttpResponse lresp;
  err = cl.ReadResponse(&lresp);
  if (err) {
    cl.Disconnect();
    return err;
  }

  if (lresp.IsEmptyBody()) {
    cl.Disconnect();
    return common::make_error("Empty body");
  }

  cl.Disconnect();
  return common::Error();
}

}  // namespace base
}  // namespace fastocloud
