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

#include "daemon/commands_info/server_info.h"

#define ONLINE_USERS_FIELD "online_users"

#define OS_FIELD "os"
#define VERSION_FIELD "version"
#define PROJECT_FIELD "project"
#define HTTP_HOST_FIELD "http_host"
#define EXPIRATION_TIME_FIELD "expiration_time"

#define ONLINE_USERS_DAEMON_FIELD "daemon"
#define ONLINE_USERS_HTTP_FIELD "http"
#define ONLINE_USERS_SUBSCRIBERS_FIELD "subscribers"

namespace fastocloud {
namespace server {
namespace service {

OnlineUsers::OnlineUsers() : OnlineUsers(0, 0, 0) {}

OnlineUsers::OnlineUsers(size_t daemon, size_t http, size_t subscribers)
    : daemon_(daemon), http_(http), subscribers_(subscribers) {}

common::Error OnlineUsers::DoDeSerialize(json_object* serialized) {
  OnlineUsers inf;
  json_object* jdaemon = nullptr;
  json_bool jdaemon_exists = json_object_object_get_ex(serialized, ONLINE_USERS_DAEMON_FIELD, &jdaemon);
  if (jdaemon_exists) {
    inf.daemon_ = json_object_get_int64(jdaemon);
  }

  json_object* jhttp = nullptr;
  json_bool jhttp_exists = json_object_object_get_ex(serialized, ONLINE_USERS_HTTP_FIELD, &jhttp);
  if (jhttp_exists) {
    inf.http_ = json_object_get_int64(jhttp);
  }

  json_object* jsubscribers = nullptr;
  json_bool jsubscribers_exists = json_object_object_get_ex(serialized, ONLINE_USERS_SUBSCRIBERS_FIELD, &jsubscribers);
  if (jsubscribers_exists) {
    inf.subscribers_ = json_object_get_int64(jsubscribers);
  }

  *this = inf;
  return common::Error();
}

common::Error OnlineUsers::SerializeFields(json_object* out) const {
  json_object_object_add(out, ONLINE_USERS_DAEMON_FIELD, json_object_new_int64(daemon_));
  json_object_object_add(out, ONLINE_USERS_HTTP_FIELD, json_object_new_int64(http_));
  json_object_object_add(out, ONLINE_USERS_SUBSCRIBERS_FIELD, json_object_new_int64(subscribers_));
  return common::Error();
}

ServerInfo::ServerInfo() : base_class(), online_users_() {}

ServerInfo::ServerInfo(cpu_load_t cpu_load,
                       gpu_load_t gpu_load,
                       const std::string& load_average,
                       size_t ram_bytes_total,
                       size_t ram_bytes_free,
                       size_t hdd_bytes_total,
                       size_t hdd_bytes_free,
                       fastotv::bandwidth_t net_bytes_recv,
                       fastotv::bandwidth_t net_bytes_send,
                       time_t uptime,
                       fastotv::timestamp_t timestamp,
                       const OnlineUsers& online_users,
                       size_t net_total_bytes_recv,
                       size_t net_total_bytes_send)
    : base_class(cpu_load,
                 gpu_load,
                 load_average,
                 ram_bytes_total,
                 ram_bytes_free,
                 hdd_bytes_total,
                 hdd_bytes_free,
                 net_bytes_recv,
                 net_bytes_send,
                 uptime,
                 timestamp,
                 net_total_bytes_recv,
                 net_total_bytes_send),
      online_users_(online_users) {}

common::Error ServerInfo::SerializeFields(json_object* out) const {
  common::Error err = base_class::SerializeFields(out);
  if (err) {
    return err;
  }

  json_object* obj = nullptr;
  err = online_users_.Serialize(&obj);
  if (err) {
    return err;
  }

  json_object_object_add(out, ONLINE_USERS_FIELD, obj);
  return common::Error();
}

common::Error ServerInfo::DoDeSerialize(json_object* serialized) {
  ServerInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jonline = nullptr;
  json_bool jonline_exists = json_object_object_get_ex(serialized, ONLINE_USERS_FIELD, &jonline);
  if (jonline_exists) {
    common::Error err = inf.online_users_.DeSerialize(jonline);
    if (err) {
      return err;
    }
  }

  *this = inf;
  return common::Error();
}

OnlineUsers ServerInfo::GetOnlineUsers() const {
  return online_users_;
}

FullServiceInfo::FullServiceInfo()
    : base_class(),
      http_host_(),
      project_(PROJECT_NAME_LOWERCASE),
      proj_ver_(PROJECT_VERSION_HUMAN),
      os_(fastotv::commands_info::OperationSystemInfo::MakeOSSnapshot()) {}

FullServiceInfo::FullServiceInfo(const common::net::HostAndPort& http_host,
                                 common::time64_t exp_time,
                                 const base_class& base)
    : base_class(base),
      http_host_(http_host),
      exp_time_(exp_time),
      project_(PROJECT_NAME_LOWERCASE),
      proj_ver_(PROJECT_VERSION_HUMAN),
      os_(fastotv::commands_info::OperationSystemInfo::MakeOSSnapshot()) {}

common::net::HostAndPort FullServiceInfo::GetHttpHost() const {
  return http_host_;
}

std::string FullServiceInfo::GetProjectVersion() const {
  return proj_ver_;
}

common::Error FullServiceInfo::DoDeSerialize(json_object* serialized) {
  FullServiceInfo inf;
  common::Error err = inf.base_class::DoDeSerialize(serialized);
  if (err) {
    return err;
  }

  json_object* jos = nullptr;
  json_bool jos_exists = json_object_object_get_ex(serialized, OS_FIELD, &jos);
  if (jos_exists) {
    common::Error err = inf.os_.DeSerialize(jos);
    if (err) {
      return err;
    }
  }

  json_object* jhttp_host = nullptr;
  json_bool jhttp_host_exists = json_object_object_get_ex(serialized, HTTP_HOST_FIELD, &jhttp_host);
  if (jhttp_host_exists) {
    common::net::HostAndPort host;
    if (common::ConvertFromString(json_object_get_string(jhttp_host), &host)) {
      inf.http_host_ = host;
    }
  }

  json_object* jexp = nullptr;
  json_bool jexp_exists = json_object_object_get_ex(serialized, EXPIRATION_TIME_FIELD, &jexp);
  if (jexp_exists) {
    inf.exp_time_ = json_object_get_int64(jexp);
  }

  json_object* jproj = nullptr;
  json_bool jproj_exists = json_object_object_get_ex(serialized, PROJECT_FIELD, &jproj);
  if (jproj_exists) {
    inf.project_ = json_object_get_string(jproj);
  }

  json_object* jproj_ver = nullptr;
  json_bool jproj_ver_exists = json_object_object_get_ex(serialized, VERSION_FIELD, &jproj_ver);
  if (jproj_ver_exists) {
    inf.proj_ver_ = json_object_get_string(jproj_ver);
  }

  *this = inf;
  return common::Error();
}

common::Error FullServiceInfo::SerializeFields(json_object* out) const {
  json_object* jos = nullptr;
  common::Error err = os_.Serialize(&jos);
  if (err) {
    return err;
  }

  std::string http_host_str = common::ConvertToString(http_host_);
  json_object_object_add(out, HTTP_HOST_FIELD, json_object_new_string(http_host_str.c_str()));
  json_object_object_add(out, EXPIRATION_TIME_FIELD, json_object_new_int64(exp_time_));
  json_object_object_add(out, PROJECT_FIELD, json_object_new_string(project_.c_str()));
  json_object_object_add(out, VERSION_FIELD, json_object_new_string(proj_ver_.c_str()));
  json_object_object_add(out, OS_FIELD, jos);
  return base_class::SerializeFields(out);
}

}  // namespace service
}  // namespace server
}  // namespace fastocloud
