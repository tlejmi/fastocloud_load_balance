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

#include "mongo/subscribers_manager.h"

#include <algorithm>
#include <memory>

#include <common/file_system/string_path_utils.h>
#include <common/sprintf.h>
#include <common/time.h>

#include <fastotv/types/input_uri.h>

#include "base/server_auth_info.h"
#include "base/subscriber_info.h"

#include "mongo/mongo2info.h"
#include "mongo/mongo_engine.h"

#define SUBSCRIBERS_COLLECTION "subscribers"
#define SERVERS_COLLECTION "services"
#define STREAMS_COLLECTION "streams"
#define SERIES_COLLECTION "series"
#define REQUESTS_COLLECTION "requests"

#define PROXY_STR "pyfastocloud_models.stream.entry.ProxyStream"
#define VOD_PROXY_STR "pyfastocloud_models.stream.entry.ProxyVodStream"
#define COD_RELAY_STR "pyfastocloud_models.stream.entry.CodRelayStream"
#define COD_ENCODE_STR "pyfastocloud_models.stream.entry.CodEncodeStream"
#define RELAY_STR "pyfastocloud_models.stream.entry.RelayStream"
#define ENCODE_STR "pyfastocloud_models.stream.entry.EncodeStream"
#define VOD_RELAY_STR "pyfastocloud_models.stream.entry.VodRelayStream"
#define VOD_ENCODE_STR "pyfastocloud_models.stream.entry.VodEncodeStream"
#define TIMESHIFT_RECORDER_STR "pyfastocloud_models.stream.entry.TimeshiftRecorderStream"
#define TIMESHIFT_PLAYER_STR "pyfastocloud_models.stream.entry.TimeshiftPlayerStream"
#define CATCHUP_STR "pyfastocloud_models.stream.entry.CatchupStream"
#define TEST_LIFE_STR "pyfastocloud_models.stream.entry.TestLifeStream"
#define CV_DATA_STR "pyfastocloud_models.stream.entry.CvDataStream"

#define FAVORITE_FIELD "favorite"
#define RECENT_FIELD "recent"
#define PRIVATE_FIELD "private"
#define LOCKED_FIELD "locked"
#define INTERRUPTION_TIME_FIELD "interruption_time"
#define USER_STREAM_ID_FIELD "sid"
#define USER_STREAMS_FIELD "streams"
#define USER_VODS_FIELD "vods"
#define USER_CATCHUPS_FIELD "catchups"
#define SERIES_FIELD "series"
#define REQUESTS_FIELD "requests"

#define SERVER_STREAMS_FIELD "streams"

#define INPUT_URL_CLS "pyfastocloud_models.common_entries.InputUrl"
#define OUTPUT_URL_CLS "pyfastocloud_models.common_entries.OutputUrl"
#define CATCHUP_USER_CLS_VALUE "pyfastocloud_models.subscriber.entry.UserStream"

#define CONTENT_REQUEST_CLS "pyfastocloud_models.content_request.entry.ContentRequest"

namespace {

enum UserStatus { USER_NOT_ACTIVE = 0, USER_ACTIVE = 1, USER_DELETED = 2 };
enum DeviceStatus { DEVICE_NOT_ACTIVE = 0, DEVICE_ACTIVE = 1, DEVICE_BANNED = 2 };

fastotv::StreamType MongoStreamType2StreamType(const char* data) {
  if (strcmp(data, PROXY_STR) == 0) {
    return fastotv::PROXY;
  } else if (strcmp(data, VOD_PROXY_STR) == 0) {
    return fastotv::VOD_PROXY;
  } else if (strcmp(data, RELAY_STR) == 0) {
    return fastotv::RELAY;
  } else if (strcmp(data, ENCODE_STR) == 0) {
    return fastotv::ENCODE;
  } else if (strcmp(data, VOD_RELAY_STR) == 0) {
    return fastotv::VOD_RELAY;
  } else if (strcmp(data, VOD_ENCODE_STR) == 0) {
    return fastotv::VOD_ENCODE;
  } else if (strcmp(data, TIMESHIFT_RECORDER_STR) == 0) {
    return fastotv::TIMESHIFT_RECORDER;
  } else if (strcmp(data, TIMESHIFT_PLAYER_STR) == 0) {
    return fastotv::TIMESHIFT_PLAYER;
  } else if (strcmp(data, CATCHUP_STR) == 0) {
    return fastotv::CATCHUP;
  } else if (strcmp(data, COD_RELAY_STR) == 0) {
    return fastotv::COD_RELAY;
  } else if (strcmp(data, COD_ENCODE_STR) == 0) {
    return fastotv::COD_ENCODE;
  } else if (strcmp(data, TEST_LIFE_STR) == 0) {
    return fastotv::TEST_LIFE;
  } else if (strcmp(data, CV_DATA_STR) == 0) {
    return fastotv::CV_DATA;
  }

  return fastotv::PROXY;
}

bool IsVod(fastotv::StreamType st) {
  return st == fastotv::VOD_RELAY || st == fastotv::VOD_ENCODE || st == fastotv::VOD_PROXY;
}

void CreateInputUrl(bson_t* result, const std::vector<fastotv::InputUri>& urls) {
  /*
      [
          {
            "uri" : "https://1239101249.rsc.cdn77.org//LS-59570-ABN/tracks-v1a1/mono.m3u8",
            "user_agent" : 0,
            "stream_link" : false,
            "proxy" : {
                "url" : "",
                "user" : "",
                "password" : "",
                "_cls" : "pyfastocloud_models.common_entries.HttpProxy"
            },
            "_cls" : "pyfastocloud_models.common_entries.InputUrl"
          }
      ]
  */
  bson_t child;
  BSON_APPEND_ARRAY_BEGIN(result, STREAM_INPUT_FIELD, &child);
  char buf[16];
  for (size_t i = 0; i < urls.size(); ++i) {
    const fastotv::InputUri iurl = urls[i];
    bson_t url;
    const char* key;
    size_t keylen = bson_uint32_to_string(i, &key, buf, sizeof(buf));
    // url
    bson_append_document_begin(&child, key, keylen, &url);
    BSON_APPEND_UTF8(&url, "_cls", INPUT_URL_CLS);
    BSON_APPEND_INT32(&url, "id", iurl.GetID());
    const std::string url_str = iurl.GetUrl().spec();
    BSON_APPEND_UTF8(&url, "uri", url_str.c_str());
    const auto ua = iurl.GetUserAgent();
    if (ua) {
      BSON_APPEND_INT32(&url, "user_agent", *ua);
    }
    // http_proxy and stream_link
    bson_append_document_end(&child, &url);
  }
  bson_append_array_end(result, &child);
}

void CreateOutputUrl(bson_t* result, const std::vector<fastotv::OutputUri>& urls) {
  /*
      [
          {
              "_cls" : "pyfastocloud_models.common_entries.OutputUrl",
              "id" : 41,
              "uri" : "http://localhost:8000/3/5e2677ebd18029a897d2716c/41/playlist.m3u8",
              "http_root" : "~/streamer/hls/3/5e2677ebd18029a897d2716c/41",
              "hls_type" : 0
          }
      ]
  */
  bson_t child;
  BSON_APPEND_ARRAY_BEGIN(result, STREAM_OUTPUT_FIELD, &child);
  char buf[16];
  for (size_t i = 0; i < urls.size(); ++i) {
    const auto out = urls[i];
    bson_t url;
    const char* key;
    size_t keylen = bson_uint32_to_string(i, &key, buf, sizeof(buf));
    bson_append_document_begin(&child, key, keylen, &url);
    BSON_APPEND_UTF8(&url, "_cls", OUTPUT_URL_CLS);
    BSON_APPEND_INT32(&url, "id", out.GetID());
    const auto url_str = out.GetUrl().spec();
    BSON_APPEND_UTF8(&url, "uri", url_str.c_str());
    const auto http_root = out.GetHttpRoot();
    if (http_root) {
      const std::string http_root_str = http_root->GetPath();
      BSON_APPEND_UTF8(&url, "http_root", http_root_str.c_str());
    }
    const auto hls_type = out.GetHlsType();
    if (hls_type) {
      BSON_APPEND_INT32(&url, "hls_type", *hls_type);
    }
    bson_append_document_end(&child, &url);
  }
  bson_append_array_end(result, &child);
}

std::vector<fastotv::OutputUri> CreateCatchupOutputUrl(
    bson_t* result,
    const std::vector<fastotv::OutputUri>& urls,
    fastotv::stream_id_t sid,
    const common::net::HostAndPort& catchup_host,
    const common::file_system::ascii_directory_string_path& catchups_http_root) {
  std::vector<fastotv::OutputUri> patched;
  for (size_t i = 0; i < urls.size(); ++i) {
    fastotv::OutputUri out = urls[i];
    const auto origin_url = out.GetUrl();

    const std::string catchup_host_str = common::ConvertToString(catchup_host);
    const std::string postfix_dir = common::MemSPrintf("%d/%s/%u", fastotv::CATCHUP, sid, out.GetID());
    const std::string url_str = common::MemSPrintf("http://%s/%s/master.m3u8", catchup_host_str, postfix_dir);
    out.SetUrl(common::uri::GURL(url_str));
    auto cat_dir = catchups_http_root.MakeDirectoryStringPath(postfix_dir);
    if (cat_dir) {
      out.SetHttpRoot(*cat_dir);
    }
    patched.push_back(out);
  }
  CreateOutputUrl(result, patched);
  return patched;
}

}  // namespace

namespace fastocloud {
namespace server {
namespace mongo {

namespace {

typedef std::unique_ptr<bson_t, MongoQueryDeleter> unique_ptr_bson_t;

UserStreamInfo makeUserStreamInfo(bson_iter_t* iter) {
  UserStreamInfo uinf;
  if (bson_iter_find(iter, FAVORITE_FIELD)) {
    uinf.favorite = bson_iter_bool(iter);
  }
  if (bson_iter_find(iter, PRIVATE_FIELD)) {
    uinf.priv = bson_iter_bool(iter);
  }
  if (bson_iter_find(iter, LOCKED_FIELD)) {
    uinf.locked = bson_iter_bool(iter);
  }
  if (bson_iter_find(iter, RECENT_FIELD)) {
    uinf.recent = bson_iter_date_time(iter);
  }
  if (bson_iter_find(iter, INTERRUPTION_TIME_FIELD)) {
    uinf.interruption_time = bson_iter_int32(iter);
  }

  return uinf;
}

common::Error AddStreamToServer(mongoc_collection_t* servers,
                                const bson_oid_t* server_oid,
                                const bson_oid_t* stream_oid) {
  bson_error_t error;
  bson_t* server_id = BCON_NEW("_id", BCON_OID(server_oid));
  bson_t* push_command = BCON_NEW("$push", "{", SERVER_STREAMS_FIELD, BCON_OID(stream_oid), "}");
  const unique_ptr_bson_t query_server_update(server_id);
  const unique_ptr_bson_t update_query_server_update(push_command);
  if (!mongoc_collection_update(servers, MONGOC_UPDATE_NONE, query_server_update.get(),
                                update_query_server_update.get(), NULL, &error)) {
    return common::make_error(error.message);
  }

  return common::Error();
}

common::Error AddContentRequestToUserArray(mongoc_collection_t* subscribers,
                                           const bson_oid_t* user_oid,
                                           const bson_oid_t* request_oid) {
  bson_error_t error;
  const unique_ptr_bson_t query_user(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query_user(BCON_NEW("$push", "{", REQUESTS_FIELD, BCON_OID(request_oid), "}"));
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query_user.get(), update_query_user.get(), NULL,
                                &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error AddStreamToUserStreamsArray(mongoc_collection_t* subscribers,
                                          const bson_oid_t* user_oid,
                                          const bson_oid_t* stream_oid) {
  bson_error_t error;
  const unique_ptr_bson_t query_user(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query_user(BCON_NEW("$push", "{", USER_STREAMS_FIELD, "{", USER_STREAM_ID_FIELD,
                                                     BCON_OID(stream_oid), FAVORITE_FIELD, BCON_BOOL(false),
                                                     PRIVATE_FIELD, BCON_BOOL(false), RECENT_FIELD, BCON_DATE_TIME(0),
                                                     INTERRUPTION_TIME_FIELD, BCON_INT32(0), "}", "}"));
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query_user.get(), update_query_user.get(), NULL,
                                &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error AddStreamToUserVodsArray(mongoc_collection_t* subscribers,
                                       const bson_oid_t* user_oid,
                                       const bson_oid_t* stream_oid) {
  bson_error_t error;
  const unique_ptr_bson_t query_user(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query_user(BCON_NEW("$push", "{", USER_VODS_FIELD, "{", USER_STREAM_ID_FIELD,
                                                     BCON_OID(stream_oid), FAVORITE_FIELD, BCON_BOOL(false),
                                                     PRIVATE_FIELD, BCON_BOOL(false), RECENT_FIELD, BCON_DATE_TIME(0),
                                                     INTERRUPTION_TIME_FIELD, BCON_INT32(0), "}", "}"));
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query_user.get(), update_query_user.get(), NULL,
                                &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error AddStreamToUserCatchupsArray(mongoc_collection_t* subscribers,
                                           const bson_oid_t* user_oid,
                                           const bson_oid_t* stream_oid) {
  bson_error_t error;
  const unique_ptr_bson_t query_user(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query_user(
      BCON_NEW("$push", "{", USER_CATCHUPS_FIELD, "{", USER_STREAM_ID_FIELD, BCON_OID(stream_oid), FAVORITE_FIELD,
               BCON_BOOL(false), PRIVATE_FIELD, BCON_BOOL(false), RECENT_FIELD, BCON_DATE_TIME(0),
               INTERRUPTION_TIME_FIELD, BCON_INT32(0), "_cls", CATCHUP_USER_CLS_VALUE, "}", "}"));
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query_user.get(), update_query_user.get(), NULL,
                                &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error RemoveStreamFromUserStreamsArray(mongoc_collection_t* subscribers,
                                               const bson_oid_t* user_oid,
                                               const bson_oid_t* stream_oid) {
  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query(
      BCON_NEW("$pull", "{", USER_STREAMS_FIELD, "{", USER_STREAM_ID_FIELD, BCON_OID(stream_oid), "}", "}"));
  bson_error_t error;
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error RemoveStreamFromUserVodsArray(mongoc_collection_t* subscribers,
                                            const bson_oid_t* user_oid,
                                            const bson_oid_t* stream_oid) {
  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query(
      BCON_NEW("$pull", "{", USER_VODS_FIELD, "{", USER_STREAM_ID_FIELD, BCON_OID(stream_oid), "}", "}"));
  bson_error_t error;
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

common::Error RemoveStreamFromUserCatchupsArray(mongoc_collection_t* subscribers,
                                                const bson_oid_t* user_oid,
                                                const bson_oid_t* stream_oid) {
  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(user_oid)));
  const unique_ptr_bson_t update_query(
      BCON_NEW("$pull", "{", USER_CATCHUPS_FIELD, "{", USER_STREAM_ID_FIELD, BCON_OID(stream_oid), "}", "}"));
  bson_error_t error;
  if (!mongoc_collection_update(subscribers, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
    return common::make_error(error.message);
  }
  return common::Error();
}

}  // namespace

SubscribersManager::SubscribersManager(base::ISubscribersObserver* observer)
    : base_class(observer),
      connections_mutex_(),
      connections_(),
      client_(nullptr),
      subscribers_(nullptr),
      servers_(nullptr),
      streams_(nullptr),
      series_(nullptr),
      requests_(nullptr),
      catchup_endpoint_() {}

void SubscribersManager::SetupCatchupsEndpoint(const base::CatchupEndpointInfo& info) {
  catchup_endpoint_ = info;
}

common::Error SubscribersManager::SendSubscriberNotification(
    const fastotv::user_id_t& uid,
    const fastotv::device_id_t& device,
    const fastotv::commands_info::NotificationTextInfo& notify) {
  std::unique_lock<std::mutex> lock(connections_mutex_);
  auto hs = connections_.find(uid);
  if (hs == connections_.end()) {
    return common::make_error("Client not found");
  }

  for (auto connection : hs->second) {
    const auto login = connection->GetLogin();
    if (login && login->GetDeviceID() == device) {
      common::ErrnoError errn = connection->SendNotification(notify);
      if (errn) {
        return common::make_error_from_errno(errn);
      }
      return common::Error();
    }
  }
  return common::make_error("Device not found");
}

std::vector<base::FrontSubscriberInfo> SubscribersManager::GetOnlineSubscribers() {
  std::unique_lock<std::mutex> lock(connections_mutex_);
  std::vector<base::FrontSubscriberInfo> result;
  for (auto clients : connections_) {
    for (const auto* client : clients.second) {
      if (client) {
        const auto login = client->MakeFrontSubscriberInfo();
        if (login) {
          result.push_back(*login);
        }
      }
    }
  }
  return result;
}

common::ErrnoError SubscribersManager::ConnectToDatabase(const std::string& mongodb_url,
                                                         const std::string& db_name,
                                                         bool lazy) {
  if (db_name.empty()) {
    return common::make_errno_error_inval();
  }

  mongoc_client_t* client = nullptr;
  common::ErrnoError err = MongoEngine::GetInstance().Connect(mongodb_url, lazy, &client);
  if (err) {
    return err;
  }

  const char* db = db_name.c_str();
  mongoc_collection_t* scollection = mongoc_client_get_collection(client, db, SUBSCRIBERS_COLLECTION);
  if (!scollection) {
    mongoc_client_destroy(client);
    return common::make_errno_error("Can't find " SUBSCRIBERS_COLLECTION " collection.", EAGAIN);
  }

  mongoc_collection_t* sercollection = mongoc_client_get_collection(client, db, SERVERS_COLLECTION);
  if (!sercollection) {
    mongoc_collection_destroy(scollection);
    mongoc_client_destroy(client);
    return common::make_errno_error("Can't find " SERVERS_COLLECTION " collection.", EAGAIN);
  }

  mongoc_collection_t* stcollection = mongoc_client_get_collection(client, db, STREAMS_COLLECTION);
  if (!stcollection) {
    mongoc_collection_destroy(sercollection);
    mongoc_collection_destroy(scollection);
    mongoc_client_destroy(client);
    return common::make_errno_error("Can't find " STREAMS_COLLECTION " collection.", EAGAIN);
  }

  mongoc_collection_t* series = mongoc_client_get_collection(client, db, SERIES_COLLECTION);
  if (!series) {
    mongoc_collection_destroy(sercollection);
    mongoc_collection_destroy(scollection);
    mongoc_client_destroy(client);
    return common::make_errno_error("Can't find " SERIES_COLLECTION " collection.", EAGAIN);
  }

  mongoc_collection_t* requests = mongoc_client_get_collection(client, db, REQUESTS_COLLECTION);
  if (!requests) {
    mongoc_collection_destroy(sercollection);
    mongoc_collection_destroy(scollection);
    mongoc_collection_destroy(series);
    mongoc_client_destroy(client);
    return common::make_errno_error("Can't find " REQUESTS_COLLECTION " collection.", EAGAIN);
  }

  servers_ = sercollection;
  streams_ = stcollection;
  subscribers_ = scollection;
  series_ = series;
  requests_ = requests;
  client_ = client;
  return common::ErrnoError();
}

common::ErrnoError SubscribersManager::Disconnect() {
  if (servers_) {
    mongoc_collection_destroy(servers_);
    servers_ = nullptr;
  }

  if (streams_) {
    mongoc_collection_destroy(streams_);
    streams_ = nullptr;
  }

  if (subscribers_) {
    mongoc_collection_destroy(subscribers_);
    subscribers_ = nullptr;
  }

  if (series_) {
    mongoc_collection_destroy(series_);
    series_ = nullptr;
  }

  if (requests_) {
    mongoc_collection_destroy(requests_);
    requests_ = nullptr;
  }

  if (client_) {
    mongoc_client_destroy(client_);
    client_ = nullptr;
  }

  return common::ErrnoError();
}

common::Error SubscribersManager::RegisterInnerConnectionByHost(base::SubscriberInfo* client,
                                                                const base::ServerDBAuthInfo& info) {
  CHECK(info.IsValid());
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  client->SetLogin(info);
  std::unique_lock<std::mutex> lock(connections_mutex_);
  connections_[info.GetUserID()].push_back(client);
  return base_class::RegisterInnerConnectionByHost(client, info);
}

common::Error SubscribersManager::UnRegisterInnerConnectionByHost(base::SubscriberInfo* client) {
  if (!client) {
    DNOTREACHED();
    return common::make_error_inval();
  }

  const auto sinf = client->GetLogin();
  if (!sinf) {
    return common::make_error_inval();
  }

  std::unique_lock<std::mutex> lock(connections_mutex_);
  auto hs = connections_.find(sinf->GetUserID());
  if (hs == connections_.end()) {
    return base_class::UnRegisterInnerConnectionByHost(client);
  }

  for (auto it = hs->second.begin(); it != hs->second.end(); ++it) {
    if (*it == client) {
      hs->second.erase(it);
      break;
    }
  }

  if (hs->second.empty()) {
    connections_.erase(hs);
  }
  return base_class::UnRegisterInnerConnectionByHost(client);
}

common::Error SubscribersManager::CheckIsLoginClient(base::SubscriberInfo* client, base::ServerDBAuthInfo* ser) const {
  if (!client || !ser) {
    return common::make_error_inval();
  }

  const auto login = client->GetLogin();
  if (!login) {
    return common::make_error("User not logged");
  }

  *ser = *login;
  return common::Error();
}

size_t SubscribersManager::GetAndUpdateOnlineUserByStreamID(fastotv::stream_id_t sid) {
  std::unique_lock<std::mutex> lock(connections_mutex_);
  size_t total = 0;
  for (auto clients : connections_) {
    for (const auto* client : clients.second) {
      if (client && client->GetCurrentStreamID() == sid) {
        total++;
      }
    }
  }

  return total;
}

common::Error SubscribersManager::ClientActivate(const fastotv::commands_info::LoginInfo& uauth,
                                                 fastotv::commands_info::DevicesInfo* dev) {
  if (!uauth.IsValid() || !dev) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  const std::string login = uauth.GetLogin();
  const unique_ptr_bson_t query(bson_new());
  BSON_APPEND_UTF8(query.get(), "email", login.c_str());
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* doc;
  if (!cursor || !mongoc_cursor_next(cursor.get(), &doc)) {
    return common::make_error("User not found");
  }

  bson_iter_t bstatus;
  if (!bson_iter_init_find(&bstatus, doc, "status") || !BSON_ITER_HOLDS_INT32(&bstatus)) {
    return common::make_error("Not found status field");
  }

  UserStatus status = static_cast<UserStatus>(bson_iter_int32(&bstatus));
  if (status == USER_NOT_ACTIVE) {
    return common::make_error("User not active");
  }

  if (status == USER_DELETED) {
    return common::make_error("User removed");
  }

  bson_iter_t bpassword;
  if (!bson_iter_init_find(&bpassword, doc, "password") || !BSON_ITER_HOLDS_UTF8(&bpassword)) {
    return common::make_error("Not found password field");
  }

  const char* password_hash = bson_iter_utf8(&bpassword, NULL);
  if (uauth.GetPassword() != password_hash) {
    return common::make_error("Invalid password");
  }

  bson_iter_t bdevices;
  if (!bson_iter_init_find(&bdevices, doc, "devices") || !BSON_ITER_HOLDS_ARRAY(&bdevices)) {
    return common::make_error("Please create device in your profile page");
  }

  fastotv::commands_info::DevicesInfo devices;
  bson_iter_t ar;
  if (bson_iter_recurse(&bdevices, &ar)) {
    while (bson_iter_next(&ar)) {
      if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
        bson_iter_t did;
        if (bson_iter_recurse(&ar, &did) && bson_iter_find(&did, "_id") && BSON_ITER_HOLDS_OID(&did)) {
          bson_iter_t bdevice_status;
          if (bson_iter_recurse(&ar, &bdevice_status) && bson_iter_find(&bdevice_status, "status") &&
              BSON_ITER_HOLDS_INT32(&bdevice_status)) {
            DeviceStatus device_status = static_cast<DeviceStatus>(bson_iter_int32(&bdevice_status));
            if (device_status == DEVICE_NOT_ACTIVE) {
              bson_iter_t dname;
              if (bson_iter_recurse(&ar, &dname) && bson_iter_find(&dname, "name") && BSON_ITER_HOLDS_UTF8(&dname)) {
                const bson_oid_t* oid = bson_iter_oid(&did);
                const fastotv::commands_info::DeviceInfo::device_t str_oid = common::ConvertToString(oid);
                std::string name_str = bson_iter_utf8(&dname, NULL);
                devices.Add(fastotv::commands_info::DeviceInfo(str_oid, name_str));
              }
            }
          }
        }
      }
    }
  }

  if (devices.Empty()) {
    return common::make_error("Please create device in your profile page");
  }

  *dev = devices;
  return common::Error();
}

common::Error SubscribersManager::ClientLogin(fastotv::user_id_t uid,
                                              const std::string& password,
                                              fastotv::device_id_t dev,
                                              base::ServerDBAuthInfo* ser) {
  if (uid.empty() || password.empty() || dev.empty()) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(uid, &oid)) {
    return common::make_error("Invalid user id");
  }

  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* doc;
  if (!cursor || !mongoc_cursor_next(cursor.get(), &doc)) {
    return common::make_error("User not found");
  }

  bson_iter_t blogin;
  if (!bson_iter_init_find(&blogin, doc, "email") || !BSON_ITER_HOLDS_UTF8(&blogin)) {
    return common::make_error("Not found email field");
  }

  bson_iter_t bexp_date;
  if (!bson_iter_init_find(&bexp_date, doc, "exp_date") || !BSON_ITER_HOLDS_DATE_TIME(&bexp_date)) {
    return common::make_error("Not exp_date field");
  }

  const fastotv::login_t login = bson_iter_utf8(&blogin, NULL);
  const fastotv::timestamp_t exp_date = bson_iter_date_time(&bexp_date);
  fastotv::commands_info::AuthInfo log(fastotv::commands_info::LoginInfo(login, password), dev);
  fastotv::commands_info::ServerAuthInfo uauth(log, exp_date);
  common::Error err = ClientLoginImpl(uid, uauth, doc);
  if (err) {
    return err;
  }

  *ser = base::ServerDBAuthInfo(uid, uauth);
  return common::Error();
}

common::Error SubscribersManager::ClientLogin(const fastotv::commands_info::AuthInfo& uauth,
                                              base::ServerDBAuthInfo* ser) {
  if (!uauth.IsValid() || !ser) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  const std::string login = uauth.GetLogin();
  const unique_ptr_bson_t query(bson_new());
  BSON_APPEND_UTF8(query.get(), "email", login.c_str());
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* doc;
  if (!cursor || !mongoc_cursor_next(cursor.get(), &doc)) {
    return common::make_error("User not found");
  }

  bson_iter_t buid;
  if (!bson_iter_init_find(&buid, doc, "_id") || !BSON_ITER_HOLDS_OID(&buid)) {
    return common::make_error("Not found _id field");
  }

  bson_iter_t bexp_date;
  if (!bson_iter_init_find(&bexp_date, doc, "exp_date") || !BSON_ITER_HOLDS_DATE_TIME(&bexp_date)) {
    return common::make_error("Not exp_date field");
  }

  const bson_oid_t* uid = bson_iter_oid(&buid);
  std::string uid_str = common::ConvertToString(uid);

  const fastotv::timestamp_t exp_date = bson_iter_date_time(&bexp_date);
  fastotv::commands_info::ServerAuthInfo suauth(uauth, exp_date);
  common::Error err = ClientLoginImpl(uid_str, suauth, doc);
  if (err) {
    return err;
  }

  *ser = base::ServerDBAuthInfo(uid_str, suauth);
  return common::Error();
}

common::Error SubscribersManager::ClientLoginImpl(fastotv::user_id_t uid,
                                                  const fastotv::commands_info::ServerAuthInfo& uauth,
                                                  const bson_t* doc) {
  bson_iter_t bstatus;
  if (!bson_iter_init_find(&bstatus, doc, "status") || !BSON_ITER_HOLDS_INT32(&bstatus)) {
    return common::make_error("Not found status field");
  }

  UserStatus status = static_cast<UserStatus>(bson_iter_int32(&bstatus));
  if (status == USER_NOT_ACTIVE) {
    return common::make_error("User not active");
  }

  if (status == USER_DELETED) {
    return common::make_error("User removed");
  }

  bson_iter_t bpassword;
  if (!bson_iter_init_find(&bpassword, doc, "password") || !BSON_ITER_HOLDS_UTF8(&bpassword)) {
    return common::make_error("Not found password field");
  }

  const char* password_hash = bson_iter_utf8(&bpassword, NULL);
  if (uauth.GetPassword() != password_hash) {
    return common::make_error("Invalid password");
  }

  fastotv::timestamp_t current_ts = common::time::current_utc_mstime();
  if (current_ts > uauth.GetExpiredDate()) {
    return common::make_error("Expired account");
  }

  bson_iter_t bdevices;
  if (!bson_iter_init_find(&bdevices, doc, "devices") || !BSON_ITER_HOLDS_ARRAY(&bdevices)) {
    return common::make_error("Please create device in your profile page");
  }

  const std::string login = uauth.GetLogin();
  bson_iter_t ar;
  bool have_devices = false;
  if (bson_iter_recurse(&bdevices, &ar)) {
    while (bson_iter_next(&ar)) {
      bson_iter_t did;
      if (BSON_ITER_HOLDS_DOCUMENT(&ar) && bson_iter_recurse(&ar, &did) && bson_iter_find(&did, "_id") &&
          BSON_ITER_HOLDS_OID(&did)) {
        have_devices = true;
        const bson_oid_t* oid = bson_iter_oid(&did);
        std::string str_oid = common::ConvertToString(oid);
        if (str_oid == uauth.GetDeviceID()) {
          bson_iter_t bdevice_status;
          if (bson_iter_recurse(&ar, &bdevice_status) && bson_iter_find(&bdevice_status, "status") &&
              BSON_ITER_HOLDS_INT32(&bdevice_status)) {
            DeviceStatus device_status = static_cast<DeviceStatus>(bson_iter_int32(&bdevice_status));
            if (device_status == DEVICE_BANNED) {
              return common::make_error("Device banned");
            }

            {
              std::unique_lock<std::mutex> lock(connections_mutex_);
              const auto hs = connections_.find(uid);
              if (hs != connections_.end()) {
                for (auto connection : hs->second) {
                  const auto login = connection->GetLogin();
                  if (login && login->GetDeviceID() == uauth.GetDeviceID()) {
                    return common::make_error("Limit connection reject");
                  }
                }
              }
            }

            if (device_status == DEVICE_NOT_ACTIVE) {
              const unique_ptr_bson_t uquery(BCON_NEW("email", login.c_str(), "devices._id", BCON_OID(oid)));
              const unique_ptr_bson_t update_query(
                  BCON_NEW("$set", "{", "devices.$.status", BCON_INT32(DEVICE_ACTIVE), "}"));
              // update({"email":"test@gmail.com", "devices._id": ObjectId("5d9c57ae9303fc2a7b2ad571")}, {"$set": {
              // "devices.$.status": NumberInt(1) }})
              bson_error_t error;
              if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, uquery.get(), update_query.get(), NULL,
                                            &error)) {
                DEBUG_LOG() << "Failed to activate device error: " << error.message;
              }
            }

            return common::Error();
          }
        }
      }
    }
  }

  if (!have_devices) {
    return common::make_error("Please create device in your profile page");
  }

  return common::make_error("Device not found");
}

common::Error SubscribersManager::ClientGetChannels(const fastotv::commands_info::AuthInfo& auth,
                                                    fastotv::commands_info::ChannelsInfo* chans,
                                                    fastotv::commands_info::VodsInfo* vods,
                                                    fastotv::commands_info::ChannelsInfo* pchans,
                                                    fastotv::commands_info::VodsInfo* pvods,
                                                    fastotv::commands_info::CatchupsInfo* catchups,
                                                    fastotv::commands_info::SeriesInfo* series,
                                                    fastotv::commands_info::ContentRequestsInfo* content_requests) {
  if (!auth.IsValid() || !chans || !vods || !pchans || !pvods || !catchups || !series || !content_requests) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_ || !series_ || !requests_) {
    return common::make_error("Not conencted to DB");
  }

  const std::string login = auth.GetLogin();
  const unique_ptr_bson_t query(bson_new());
  BSON_APPEND_UTF8(query.get(), "email", login.c_str());
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* doc;
  if (!cursor || !mongoc_cursor_next(cursor.get(), &doc)) {
    return common::make_error("User not found");
  }

  bson_iter_t bstreams;
  fastotv::commands_info::ChannelsInfo lchans;
  fastotv::commands_info::ChannelsInfo lpchans;
  if (bson_iter_init_find(&bstreams, doc, USER_STREAMS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bstreams)) {
    bson_iter_t ar;
    if (bson_iter_recurse(&bstreams, &ar)) {
      while (bson_iter_next(&ar)) {
        if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
          uint32_t len;
          const uint8_t* buf;
          bson_iter_document(&ar, &len, &buf);
          bson_t rec;
          bson_init_static(&rec, buf, len);
          {
            bson_iter_t iter;
            bson_iter_init(&iter, &rec);
            if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
              const bson_oid_t* oid = bson_iter_oid(&iter);
              const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
              const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                  mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
              const bson_t* sdoc;
              if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                bson_iter_t bcls;
                if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                  continue;
                }

                UserStreamInfo uinf = makeUserStreamInfo(&iter);
                fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                fastotv::commands_info::ChannelInfo ch;
                bool visible = false;
                if (MakeChannelInfo(sdoc, st, uinf, &ch, &visible) && visible) {
                  if (uinf.priv) {
                    lpchans.Add(ch);
                  } else {
                    lchans.Add(ch);
                  }
                }
              }
            }
          }
          bson_destroy(&rec);
        }
      }
    }
  }

  fastotv::commands_info::VodsInfo lvods;
  fastotv::commands_info::VodsInfo lpvods;
  if (bson_iter_init_find(&bstreams, doc, USER_VODS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bstreams)) {
    bson_iter_t ar;
    if (bson_iter_recurse(&bstreams, &ar)) {
      while (bson_iter_next(&ar)) {
        if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
          uint32_t len;
          const uint8_t* buf;
          bson_iter_document(&ar, &len, &buf);
          bson_t rec;
          bson_init_static(&rec, buf, len);
          {
            bson_iter_t iter;
            bson_iter_init(&iter, &rec);
            if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
              const bson_oid_t* oid = bson_iter_oid(&iter);
              const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
              const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                  mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
              const bson_t* sdoc;
              if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                bson_iter_t bcls;
                if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                  continue;
                }

                UserStreamInfo uinf = makeUserStreamInfo(&iter);
                fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                fastotv::commands_info::VodInfo ch;
                bool visible = false;
                if (MakeVodInfo(sdoc, st, uinf, &ch, &visible)) {
                  if (uinf.priv) {
                    lpvods.Add(ch);
                  } else {
                    lvods.Add(ch);
                  }
                }
              }
            }
          }
          bson_destroy(&rec);
        }
      }
    }
  }

  fastotv::commands_info::CatchupsInfo lcatchups;
  if (bson_iter_init_find(&bstreams, doc, USER_CATCHUPS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bstreams)) {
    bson_iter_t ar;
    if (bson_iter_recurse(&bstreams, &ar)) {
      while (bson_iter_next(&ar)) {
        if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
          uint32_t len;
          const uint8_t* buf;
          bson_iter_document(&ar, &len, &buf);
          bson_t rec;
          bson_init_static(&rec, buf, len);
          {
            bson_iter_t iter;
            bson_iter_init(&iter, &rec);
            if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
              const bson_oid_t* oid = bson_iter_oid(&iter);
              const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
              const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                  mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
              const bson_t* sdoc;
              if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                bson_iter_t bcls;
                if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                  continue;
                }

                UserStreamInfo uinf = makeUserStreamInfo(&iter);
                fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                fastotv::commands_info::CatchupInfo ch;
                bool visible = false;
                if (MakeCatchupInfo(sdoc, st, uinf, &ch, &visible)) {
                  lcatchups.Add(ch);
                }
              }
            }
          }
          bson_destroy(&rec);
        }
      }
    }
  }

  fastotv::commands_info::SeriesInfo lseries;
  bson_iter_t bseries;
  if (bson_iter_init_find(&bseries, doc, SERIES_FIELD) && BSON_ITER_HOLDS_ARRAY(&bseries)) {
    bson_iter_t ar;
    if (bson_iter_recurse(&bseries, &ar)) {
      while (bson_iter_next(&ar)) {
        const bson_oid_t* sid = bson_iter_oid(&ar);
        const unique_ptr_bson_t series_query(BCON_NEW("_id", BCON_OID(sid)));
        const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
            mongoc_collection_find(series_, MONGOC_QUERY_NONE, 0, 0, 0, series_query.get(), NULL, NULL));
        const bson_t* sdoc;
        if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
          fastotv::commands_info::SerialInfo ser;
          bool visible = false;
          if (MakeSerialInfo(sdoc, &ser, &visible)) {
            // stable views
            auto episodes = ser.GetEpisodes();
            auto vods_array = lvods.Get();
            size_t view_counts = 0;
            for (auto episode : episodes) {
              for (auto serial : vods_array) {
                if (serial.IsSerial() && serial.GetStreamID() == episode) {
                  view_counts += serial.GetViewCount();
                  break;
                }
              }
            }
            ser.SetViewCount(view_counts);
            lseries.Add(ser);
          }
        }
      }
    }
  }

  fastotv::commands_info::ContentRequestsInfo lcreq;
  bson_iter_t brequests;
  if (bson_iter_init_find(&brequests, doc, REQUESTS_FIELD) && BSON_ITER_HOLDS_ARRAY(&brequests)) {
    bson_iter_t ar;
    if (bson_iter_recurse(&brequests, &ar)) {
      while (bson_iter_next(&ar)) {
        const bson_oid_t* sid = bson_iter_oid(&ar);
        const unique_ptr_bson_t requests_query(BCON_NEW("_id", BCON_OID(sid)));
        const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
            mongoc_collection_find(requests_, MONGOC_QUERY_NONE, 0, 0, 0, requests_query.get(), NULL, NULL));
        const bson_t* sdoc;
        if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
          fastotv::commands_info::ContentRequestInfo cont;
          if (MakeContentRequestInfo(sdoc, &cont)) {
            lcreq.Add(cont);
          }
        }
      }
    }
  }

  *vods = lvods;
  *chans = lchans;
  *pchans = lpchans;
  *pvods = lpvods;
  *catchups = lcatchups;
  *series = lseries;
  *content_requests = lcreq;
  DEBUG_LOG() << "Vods: " << lvods.Size() << " Channels: " << lchans.Size() << " PChannels: " << lpchans.Size()
              << " PVods: " << lpvods.Size() << " Catchups: " << lcatchups.Size() << " Series: " << lseries.Size()
              << " Content requests: " << lcreq.Size();
  return common::Error();
}

common::Error SubscribersManager::ClientFindHttpDirectoryOrUrlForChannel(const fastotv::commands_info::AuthInfo& auth,
                                                                         fastotv::stream_id_t sid,
                                                                         fastotv::channel_id_t cid,
                                                                         http_directory_t* directory,
                                                                         common::uri::GURL* url) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id || !directory || !url) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  const std::string login = auth.GetLogin();
  const unique_ptr_bson_t query(bson_new());
  BSON_APPEND_UTF8(query.get(), "email", login.c_str());
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* doc;
  if (!cursor || !mongoc_cursor_next(cursor.get(), &doc)) {
    return common::make_error("User not found");
  }

  {
    bson_iter_t bstreams;
    if (bson_iter_init_find(&bstreams, doc, USER_STREAMS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bstreams)) {
      bson_iter_t ar;
      if (bson_iter_recurse(&bstreams, &ar)) {
        while (bson_iter_next(&ar)) {
          if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
            uint32_t len;
            const uint8_t* buf;
            bson_iter_document(&ar, &len, &buf);
            bson_t rec;
            bson_init_static(&rec, buf, len);
            {
              bson_iter_t iter;
              bson_iter_init(&iter, &rec);
              if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
                const bson_oid_t* oid = bson_iter_oid(&iter);
                std::string sid_str = common::ConvertToString(oid);
                if (sid_str == sid) {
                  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
                  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
                  const bson_t* sdoc;
                  if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                    bson_iter_t bcls;
                    if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                      return common::make_error("Invalid stream");
                    }

                    fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                    bool is_proxy = st == fastotv::PROXY;

                    if (is_proxy) {
                      common::uri::GURL lurl;
                      if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                        *url = lurl;
                        return common::Error();
                      }
                    } else {
                      http_directory_t ldir;
                      if (GetHttpRootFromStream(sdoc, st, cid, &ldir)) {
                        if (common::file_system::is_directory_exist(ldir.GetPath())) {
                          *directory = ldir;
                          return common::Error();
                        }

                        common::uri::GURL lurl;
                        if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                          *url = lurl;
                          return common::Error();
                        }
                      }
                    }
                    return common::make_error("Cant parse stream urls");
                  }
                  return common::make_error("Stream type not supported");
                }
              }
            }
            bson_destroy(&rec);
          }
        }
      }
    }
  }

  {
    bson_iter_t bvods;
    if (bson_iter_init_find(&bvods, doc, USER_VODS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bvods)) {
      bson_iter_t ar;
      if (bson_iter_recurse(&bvods, &ar)) {
        while (bson_iter_next(&ar)) {
          if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
            uint32_t len;
            const uint8_t* buf;
            bson_iter_document(&ar, &len, &buf);
            bson_t rec;
            bson_init_static(&rec, buf, len);
            {
              bson_iter_t iter;
              bson_iter_init(&iter, &rec);
              if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
                const bson_oid_t* oid = bson_iter_oid(&iter);
                std::string sid_str = common::ConvertToString(oid);
                if (sid_str == sid) {
                  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
                  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
                  const bson_t* sdoc;
                  if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                    bson_iter_t bcls;
                    if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                      return common::make_error("Invalid stream");
                    }

                    fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                    bool is_proxy = st == fastotv::VOD_PROXY;

                    if (is_proxy) {
                      common::uri::GURL lurl;
                      if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                        *url = lurl;
                        return common::Error();
                      }
                    } else {
                      http_directory_t ldir;
                      if (GetHttpRootFromStream(sdoc, st, cid, &ldir)) {
                        if (common::file_system::is_directory_exist(ldir.GetPath())) {
                          *directory = ldir;
                          return common::Error();
                        }

                        common::uri::GURL lurl;
                        if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                          *url = lurl;
                          return common::Error();
                        }
                      }
                    }
                    return common::make_error("Cant parse stream urls");
                  }
                  return common::make_error("Stream type not supported");
                }
              }
            }
            bson_destroy(&rec);
          }
        }
      }
    }
  }

  {
    bson_iter_t bcatchups;
    if (bson_iter_init_find(&bcatchups, doc, USER_CATCHUPS_FIELD) && BSON_ITER_HOLDS_ARRAY(&bcatchups)) {
      bson_iter_t ar;
      if (bson_iter_recurse(&bcatchups, &ar)) {
        while (bson_iter_next(&ar)) {
          if (BSON_ITER_HOLDS_DOCUMENT(&ar)) {
            uint32_t len;
            const uint8_t* buf;
            bson_iter_document(&ar, &len, &buf);
            bson_t rec;
            bson_init_static(&rec, buf, len);
            {
              bson_iter_t iter;
              bson_iter_init(&iter, &rec);
              if (bson_iter_find(&iter, USER_STREAM_ID_FIELD)) {
                const bson_oid_t* oid = bson_iter_oid(&iter);
                std::string sid_str = common::ConvertToString(oid);
                if (sid_str == sid) {
                  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(oid)));
                  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
                      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
                  const bson_t* sdoc;
                  if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
                    bson_iter_t bcls;
                    if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
                      return common::make_error("Invalid stream");
                    }

                    fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
                    bool is_proxy = (st == fastotv::PROXY || st == fastotv::VOD_PROXY);

                    if (is_proxy) {
                      common::uri::GURL lurl;
                      if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                        *url = lurl;
                        return common::Error();
                      }
                    } else {
                      bool is_requst_streams = st == fastotv::COD_RELAY || st == fastotv::COD_ENCODE ||
                                               st == fastotv::VOD_ENCODE || st == fastotv::VOD_RELAY;
                      if (!is_requst_streams) {
                        http_directory_t ldir;
                        if (GetHttpRootFromStream(sdoc, st, cid, &ldir)) {
                          if (common::file_system::is_directory_exist(ldir.GetPath())) {
                            *directory = ldir;
                            return common::Error();
                          }
                        }
                      }
                      common::uri::GURL lurl;
                      if (GetUrlFromStream(sdoc, st, cid, &lurl)) {
                        *url = lurl;
                        return common::Error();
                      }
                    }
                    return common::make_error("Cant parse stream urls");
                  }
                  return common::make_error("Stream type not supported");
                }
              }
            }
            bson_destroy(&rec);
          }
        }
      }
    }
  }

  return common::make_error("Stream not found");
}

common::Error SubscribersManager::SetFavorite(const base::ServerDBAuthInfo& auth,
                                              const fastotv::commands_info::FavoriteInfo& favorite) {
  if (!favorite.IsValid()) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t sid;
  if (!common::ConvertFromString(favorite.GetChannel(), &sid)) {
    return common::make_error("Invalid stream id");
  }

  const bson_t* sdoc;
  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&sid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
  if (!stream_cursor || !mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
    return common::make_error("Stream not found");
  }

  bson_iter_t bcls;
  if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
    return common::make_error("Invalid stream");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
  if (IsVod(st)) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_VODS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_VODS_FIELD ".$." FAVORITE_FIELD, BCON_BOOL(favorite.GetFavorite()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set favorite error: " << error.message;
    }
  } else if (st == fastotv::CATCHUP) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_CATCHUPS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_CATCHUPS_FIELD ".$." FAVORITE_FIELD, BCON_BOOL(favorite.GetFavorite()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set favorite error: " << error.message;
    }
  } else {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_STREAMS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_STREAMS_FIELD ".$." FAVORITE_FIELD, BCON_BOOL(favorite.GetFavorite()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set favorite error: " << error.message;
    }
  }

  return common::Error();
}

common::Error SubscribersManager::SetRecent(const base::ServerDBAuthInfo& auth,
                                            const fastotv::commands_info::RecentStreamTimeInfo& recent) {
  if (!recent.IsValid()) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t sid;
  if (!common::ConvertFromString(recent.GetChannel(), &sid)) {
    return common::make_error("Invalid stream id");
  }

  const bson_t* sdoc;
  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&sid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
  if (!stream_cursor || !mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
    return common::make_error("Stream not found");
  }

  bson_iter_t bcls;
  if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
    return common::make_error("Invalid stream");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
  bson_error_t error;
  if (IsVod(st)) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_VODS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_VODS_FIELD ".$." RECENT_FIELD, BCON_DATE_TIME(recent.GetTimestamp()), "}"));
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set recent error: " << error.message;
    }
  } else if (st == fastotv::CATCHUP) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_CATCHUPS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_CATCHUPS_FIELD ".$." RECENT_FIELD, BCON_DATE_TIME(recent.GetTimestamp()), "}"));
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set recent error: " << error.message;
    }
  } else {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_STREAMS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_STREAMS_FIELD ".$." RECENT_FIELD, BCON_DATE_TIME(recent.GetTimestamp()), "}"));
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set recent error: " << error.message;
    }
  }

  const unique_ptr_bson_t inc_query(BCON_NEW("$inc", "{", STREAM_VIEW_COUNT_FIELD, BCON_INT32(1), "}"));
  if (!mongoc_collection_update(streams_, MONGOC_UPDATE_NONE, stream_query.get(), inc_query.get(), NULL, &error)) {
    DEBUG_LOG() << "Can't increment view count: " << error.message;
  }

  return common::Error();
}

common::Error SubscribersManager::SetInterruptTime(const base::ServerDBAuthInfo& auth,
                                                   const fastotv::commands_info::InterruptStreamTimeInfo& inter) {
  if (!inter.IsValid()) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t sid;
  if (!common::ConvertFromString(inter.GetChannel(), &sid)) {
    return common::make_error("Invalid stream id");
  }

  const bson_t* sdoc;
  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&sid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
  if (!stream_cursor || !mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
    return common::make_error("Stream not found");
  }

  bson_iter_t bcls;
  if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
    return common::make_error("Invalid stream");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));

  if (IsVod(st)) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_VODS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_VODS_FIELD ".$." INTERRUPTION_TIME_FIELD, BCON_INT32(inter.GetTime()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set interrupt time error: " << error.message;
    }
  } else if (st == fastotv::CATCHUP) {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_CATCHUPS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_CATCHUPS_FIELD ".$." INTERRUPTION_TIME_FIELD, BCON_INT32(inter.GetTime()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set interrupt time error: " << error.message;
    }
  } else {
    const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_STREAMS_FIELD ".sid", BCON_OID(&sid)));
    const unique_ptr_bson_t update_query(
        BCON_NEW("$set", "{", USER_STREAMS_FIELD ".$." INTERRUPTION_TIME_FIELD, BCON_INT32(inter.GetTime()), "}"));
    bson_error_t error;
    if (!mongoc_collection_update(subscribers_, MONGOC_UPDATE_NONE, query.get(), update_query.get(), NULL, &error)) {
      DEBUG_LOG() << "Failed to set interrupt time error: " << error.message;
    }
  }

  return common::Error();
}

common::Error SubscribersManager::FindStream(const base::ServerDBAuthInfo& auth,
                                             fastotv::stream_id_t sid,
                                             fastotv::commands_info::ChannelInfo* chan) const {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id || !chan) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_STREAMS_FIELD ".sid", BCON_OID(&bsid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_user_cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* sdoc;
  if (stream_user_cursor && mongoc_cursor_next(stream_user_cursor.get(), &sdoc)) {
    bson_iter_t iter;
    bson_iter_init(&iter, sdoc);
    UserStreamInfo uinf = makeUserStreamInfo(&iter);

    const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&bsid)));
    const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
        mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
    if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
      bson_iter_t bcls;
      if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
        return common::make_error("Invalid stream");
      }

      fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
      if (IsVod(st)) {
      } else {
        fastotv::commands_info::ChannelInfo ch;
        bool visible = false;
        if (MakeChannelInfo(sdoc, st, uinf, &ch, &visible)) {
          *chan = ch;
          return common::Error();
        }
      }
    }
  }
  return common::make_error("Stream not found");
}

common::Error SubscribersManager::FindVod(const base::ServerDBAuthInfo& auth,
                                          fastotv::stream_id_t sid,
                                          fastotv::commands_info::VodInfo* vod) const {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id || !vod) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_VODS_FIELD ".sid", BCON_OID(&bsid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_user_cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* sdoc;
  if (stream_user_cursor && mongoc_cursor_next(stream_user_cursor.get(), &sdoc)) {
    bson_iter_t iter;
    bson_iter_init(&iter, sdoc);
    UserStreamInfo uinf = makeUserStreamInfo(&iter);

    const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&bsid)));
    const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
        mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
    if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
      bson_iter_t bcls;
      if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
        return common::make_error("Invalid stream");
      }

      fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
      if (IsVod(st)) {
        fastotv::commands_info::VodInfo ch;
        bool visible = false;
        if (MakeVodInfo(sdoc, st, uinf, &ch, &visible)) {
          *vod = ch;
          return common::Error();
        }
      }
    }
  }
  return common::make_error("Stream not found");
}

common::Error SubscribersManager::FindCatchup(const base::ServerDBAuthInfo& auth,
                                              fastotv::stream_id_t sid,
                                              fastotv::commands_info::CatchupInfo* cat) const {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id || !cat) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  const unique_ptr_bson_t query(BCON_NEW("_id", BCON_OID(&oid), USER_CATCHUPS_FIELD ".sid", BCON_OID(&bsid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_user_cursor(
      mongoc_collection_find(subscribers_, MONGOC_QUERY_NONE, 0, 0, 0, query.get(), NULL, NULL));
  const bson_t* sdoc;
  if (stream_user_cursor && mongoc_cursor_next(stream_user_cursor.get(), &sdoc)) {
    bson_iter_t iter;
    bson_iter_init(&iter, sdoc);
    UserStreamInfo uinf = makeUserStreamInfo(&iter);

    const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&bsid)));
    const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
        mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
    if (stream_cursor && mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
      bson_iter_t bcls;
      if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
        return common::make_error("Invalid stream");
      }

      fastotv::StreamType st = MongoStreamType2StreamType(bson_iter_utf8(&bcls, NULL));
      if (IsVod(st)) {
      } else {
        fastotv::commands_info::CatchupInfo ch;
        bool visible = false;
        if (MakeCatchupInfo(sdoc, st, uinf, &ch, &visible)) {
          *cat = ch;
          return common::Error();
        }
      }
    }
  }
  return common::make_error("Stream not found");
}

common::Error SubscribersManager::CreateCatchup(const base::ServerDBAuthInfo& auth,
                                                fastotv::stream_id_t sid,
                                                const std::string& title,
                                                fastotv::timestamp_t start,
                                                fastotv::timestamp_t stop,
                                                std::string* serverid,
                                                fastotv::commands_info::CatchupInfo* cat,
                                                bool* is_created) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id || !serverid || !cat || !is_created) {
    return common::make_error_inval();
  }

  if (!subscribers_ || !streams_ || !servers_) {
    return common::make_error("Not conencted to DB");
  }

  fastotv::commands_info::ChannelInfo ch;
  common::Error err = FindStream(auth, sid, &ch);
  if (err) {
    return err;
  }

  fastotv::commands_info::CatchupInfo catchup;
  err = CreateOrFindCatchup(ch, title, start, stop, serverid, &catchup, is_created);
  if (err) {
    return err;
  }

  *cat = catchup;
  return common::Error();
}

common::Error SubscribersManager::RemoveUserStream(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = RemoveStreamFromUserStreamsArray(subscribers_, &oid, &bsid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::AddUserStream(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = AddStreamToUserStreamsArray(subscribers_, &oid, &oid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::RemoveUserVod(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = RemoveStreamFromUserVodsArray(subscribers_, &oid, &bsid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::AddUserVod(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = AddStreamToUserVodsArray(subscribers_, &oid, &bsid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::RemoveUserCatchup(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = RemoveStreamFromUserCatchupsArray(subscribers_, &oid, &bsid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::AddUserCatchup(const base::ServerDBAuthInfo& auth, fastotv::stream_id_t sid) {
  if (!auth.IsValid() || sid == fastotv::invalid_stream_id) {
    return common::make_error_inval();
  }

  if (!subscribers_) {
    return common::make_error("Not conencted to DB");
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  common::Error err = AddStreamToUserCatchupsArray(subscribers_, &oid, &bsid);
  if (err) {
    return err;
  }

  return common::Error();
}

common::Error SubscribersManager::CreateRequestContent(const base::ServerDBAuthInfo& auth,
                                                       const fastotv::commands_info::CreateContentRequestInfo& request,
                                                       fastotv::commands_info::ContentRequestInfo* cont) {
  if (!auth.IsValid() || !cont) {
    return common::make_error_inval();
  }

  bson_oid_t oid;
  if (!common::ConvertFromString(auth.GetUserID(), &oid)) {
    return common::make_error("Invalid user id");
  }

  // create request
  bson_oid_t requestid;
  bson_oid_init(&requestid, NULL);
  std::string cid_str = common::ConvertToString(&requestid);

  const unique_ptr_bson_t doc(BCON_NEW("_id", BCON_OID(&requestid)));
  const std::string title = request.GetText();
  BSON_APPEND_UTF8(doc.get(), CONTENT_REQUEST_TITLE_FIELD, title.c_str());
  BSON_APPEND_UTF8(doc.get(), CONTENT_REQUEST_CLS_FIELD, CONTENT_REQUEST_CLS);
  BSON_APPEND_INT32(doc.get(), CONTENT_REQUEST_STATUS_FIELD, request.GetStatus());
  BSON_APPEND_INT32(doc.get(), CONTENT_REQUEST_TYPE_FIELD, request.GetType());

  bson_error_t error;
  if (!mongoc_collection_insert(requests_, MONGOC_INSERT_NONE, doc.get(), NULL, &error)) {
    DEBUG_LOG() << "Failed create content request error: " << error.message;
    return common::make_error(error.message);
  }

  common::Error err = AddContentRequestToUserArray(subscribers_, &oid, &requestid);
  if (err) {
    return err;
  }

  *cont =
      fastotv::commands_info::ContentRequestInfo(cid_str, request.GetText(), request.GetType(), request.GetStatus());
  return common::Error();
}

common::Error SubscribersManager::CreateOrFindCatchup(const fastotv::commands_info::ChannelInfo& based_on,
                                                      const std::string& title,
                                                      fastotv::timestamp_t start,
                                                      fastotv::timestamp_t stop,
                                                      std::string* serverid,
                                                      fastotv::commands_info::CatchupInfo* cat,
                                                      bool* is_created) {
  auto epg = based_on.GetEpg();
  epg.ClearPrograms();
  epg.SetDisplayName(title);

  fastotv::commands_info::CatchupInfo copy(based_on.GetStreamID(), based_on.GetGroups(), based_on.GetIARC(),
                                           based_on.GetFavorite(), based_on.GetRecent(), based_on.GetInterruptionTime(),
                                           epg, based_on.IsEnableAudio(), based_on.IsEnableVideo(), based_on.GetParts(),
                                           0, false, based_on.GetMetaUrls(), start, stop);

  const auto parts = based_on.GetParts();
  if (!parts.empty()) {
    for (size_t i = 0; i < parts.size(); ++i) {
      bson_oid_t sid;
      fastotv::stream_id_t chan = parts[i];
      if (!common::ConvertFromString(chan, &sid)) {
        continue;
      }

      const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&sid)));
      const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
          mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
      const bson_t* sdoc;
      if (!stream_cursor || !mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
        continue;
      }

      bson_iter_t bname;
      if (!bson_iter_init_find(&bname, sdoc, STREAM_NAME_FIELD) || !BSON_ITER_HOLDS_UTF8(&bname)) {
        continue;
      }

      bson_iter_t bstart;
      if (!bson_iter_init_find(&bstart, sdoc, CATCHUP_START_FIELD) || !BSON_ITER_HOLDS_DATE_TIME(&bstart)) {
        continue;
      }

      bson_iter_t bstop;
      if (!bson_iter_init_find(&bstop, sdoc, CATCHUP_STOP_FIELD) || !BSON_ITER_HOLDS_DATE_TIME(&bstop)) {
        continue;
      }

      const char* stream_name = bson_iter_utf8(&bname, NULL);
      fastotv::timestamp_t stream_start = bson_iter_date_time(&bstart);
      fastotv::timestamp_t stream_stop = bson_iter_date_time(&bstop);
      if (stream_name == title && stream_start == start && stop == stream_stop) {
        UserStreamInfo uinf;
        fastotv::commands_info::CatchupInfo orig;
        bool visible = false;
        if (MakeCatchupInfo(sdoc, fastotv::CATCHUP, uinf, &orig, &visible)) {
          INFO_LOG() << "Cached catchup: " << orig.GetStreamID();
          *cat = orig;
          *is_created = false;
          return common::Error();
        }
      }
    }
  }

  if (!catchup_endpoint_.IsValid()) {
    return common::make_error("Service not prepared for catchups, skiping request");
  }

  const std::string sid = based_on.GetStreamID();
  bson_oid_t bsid;
  if (!common::ConvertFromString(sid, &bsid)) {
    return common::make_error("Invalid stream id");
  }

  const unique_ptr_bson_t stream_query(BCON_NEW("_id", BCON_OID(&bsid)));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_cursor(
      mongoc_collection_find(streams_, MONGOC_QUERY_NONE, 0, 0, 0, stream_query.get(), NULL, NULL));
  const bson_t* sdoc;
  if (!stream_cursor || !mongoc_cursor_next(stream_cursor.get(), &sdoc)) {
    return common::make_error("Stream not found");
  }

  const unique_ptr_bson_t server_stream_query(
      BCON_NEW(SERVER_STREAMS_FIELD, "{", "$elemMatch", "{", "$eq", BCON_OID(&bsid), "}", "}"));
  const std::unique_ptr<mongoc_cursor_t, MongoCursorDeleter> stream_server_cursor(
      mongoc_collection_find(servers_, MONGOC_QUERY_NONE, 0, 0, 0, server_stream_query.get(), NULL, NULL));
  const bson_t* server_sdoc;
  if (!stream_server_cursor || !mongoc_cursor_next(stream_server_cursor.get(), &server_sdoc)) {
    return common::make_error("Server not found");
  }

  bson_iter_t server_id;
  if (!bson_iter_init_find(&server_id, server_sdoc, "_id") || !BSON_ITER_HOLDS_OID(&server_id)) {
    return common::make_error("Invalid stream");
  }

  bson_iter_t bcls;
  if (!bson_iter_init_find(&bcls, sdoc, STREAM_CLS_FIELD) || !BSON_ITER_HOLDS_UTF8(&bcls)) {
    return common::make_error("Invalid stream");
  }

  bson_iter_t sdoc_iter;
  if (!bson_iter_init_find(&sdoc_iter, sdoc, STREAM_OUTPUT_FIELD) || !BSON_ITER_HOLDS_ARRAY(&sdoc_iter)) {
    return common::make_error("Invalid stream");
  }

  std::vector<fastotv::OutputUri> output_urls;
  if (!GetOutputUrlData(&sdoc_iter, &output_urls)) {
    return common::make_error("Invalid stream");
  }

  // create catchup
  bson_oid_t catchupid;
  bson_oid_init(&catchupid, NULL);
  std::string cid_str = common::ConvertToString(&catchupid);

  const unique_ptr_bson_t doc(BCON_NEW("_id", BCON_OID(&catchupid)));

  BSON_APPEND_UTF8(doc.get(), STREAM_CLS_FIELD, CATCHUP_STR);
  bson_append_now_utc(doc.get(), STREAM_CREATE_DATE_FIELD, SIZEOFMASS(STREAM_CREATE_DATE_FIELD) - 1);
  BSON_APPEND_UTF8(doc.get(), STREAM_NAME_FIELD, title.c_str());
  const unique_ptr_bson_t bgroups(bson_new());
  bson_t child;
  BSON_APPEND_ARRAY_BEGIN(bgroups.get(), "languages", &child);
  const auto groups = copy.GetGroups();
  char buf[16];
  for (size_t i = 0; i < groups.size(); ++i) {
    const char* key;
    size_t keylen = bson_uint32_to_string(i, &key, buf, sizeof(buf));
    bson_append_utf8(&child, key, keylen, groups[i].c_str(), -1);
  }
  bson_append_array_end(bgroups.get(), &child);
  BSON_APPEND_ARRAY(doc.get(), STREAM_GROUPS_FIELD, bgroups.get());
  std::string tvg_id = epg.GetTvgID();
  BSON_APPEND_UTF8(doc.get(), CHANNEL_TVG_ID_FIELD, tvg_id.c_str());
  std::string tvg_name;
  BSON_APPEND_UTF8(doc.get(), CHANNEL_TVG_NAME_FIELD, tvg_name.c_str());
  std::string tvg_logo = epg.GetIconUrl().spec();
  BSON_APPEND_UTF8(doc.get(), CHANNEL_TVG_LOGO_FIELD, tvg_logo.c_str());
  double price = 0;
  BSON_APPEND_DOUBLE(doc.get(), STREAM_PRICE_FIELD, price);
  bool visible = true;
  BSON_APPEND_BOOL(doc.get(), STREAM_VISIBLE_FIELD, visible);
  int iarc = copy.GetIARC();
  BSON_APPEND_INT32(doc.get(), STREAM_IARC_FIELD, iarc);
  BSON_APPEND_INT32(doc.get(), STREAM_VIEW_COUNT_FIELD, 0);
  const unique_ptr_bson_t bparts(bson_new());
  BSON_APPEND_ARRAY(doc.get(), STREAM_PARTS_FIELD, bparts.get());
  const auto caturls = CreateCatchupOutputUrl(doc.get(), output_urls, cid_str, catchup_endpoint_.catchups_host,
                                              catchup_endpoint_.catchups_http_root);
  std::vector<common::uri::GURL> true_catchups_urls = details::MakeUrlsFromOutput(caturls);
  int log_level = common::logging::LOG_LEVEL_INFO;
  BSON_APPEND_INT32(doc.get(), STREAM_LOG_LEVEL_FIELD, log_level);
  std::vector<fastotv::InputUri> catchup_inputs;
  for (size_t i = 0; i < output_urls.size(); ++i) {
    auto ourl = output_urls[i];
    fastotv::InputUri iurl(ourl.GetID(), ourl.GetUrl());
    catchup_inputs.push_back(iurl);
  }
  CreateInputUrl(doc.get(), catchup_inputs);
  BSON_APPEND_BOOL(doc.get(), STREAM_HAVE_VIDEO_FIELD, based_on.IsEnableVideo());
  BSON_APPEND_BOOL(doc.get(), STREAM_HAVE_AUDIO_FIELD, based_on.IsEnableAudio());
  // int audio_select = -1;
  // BSON_APPEND_INT32(doc.get(), STREAM_AUDIO_SELECT_FIELD, audio_select);
  bool loop = false;
  BSON_APPEND_BOOL(doc.get(), STREAM_LOOP_FIELD, loop);
  int restart_attempts = 10;
  BSON_APPEND_INT32(doc.get(), STREAM_RESTART_ATTEMPTS_FIELD, restart_attempts);
  int auto_exit = (stop - start) / 1000;
  BSON_APPEND_INT32(doc.get(), STREAM_AUTO_EXIT_FIELD, auto_exit);
  std::string extra_args = "{}";
  BSON_APPEND_UTF8(doc.get(), STREAM_EXTRA_CONFIG_ARGS_FIELD, extra_args.c_str());
  std::string video_parser = "h264parse";
  BSON_APPEND_UTF8(doc.get(), STREAM_VIDEO_PARSER_FIELD, video_parser.c_str());
  std::string audio_parser = "aacparse";
  BSON_APPEND_UTF8(doc.get(), STREAM_AUDIO_PARSER_FIELD, audio_parser.c_str());
  int chunk_duration = 12;
  BSON_APPEND_INT32(doc.get(), TIMESHIFT_CHUNK_DURATION_FIELD, chunk_duration);
  int life_time = 43200;
  BSON_APPEND_INT32(doc.get(), TIMESHIFT_CHUNK_LIFE_TIME_FIELD, life_time);
  BSON_APPEND_DATE_TIME(doc.get(), CATCHUP_START_FIELD, start);
  BSON_APPEND_DATE_TIME(doc.get(), CATCHUP_STOP_FIELD, stop);

  bson_error_t error;
  if (!mongoc_collection_insert(streams_, MONGOC_INSERT_NONE, doc.get(), NULL, &error)) {
    DEBUG_LOG() << "Failed create catchup error: " << error.message;
    return common::make_error(error.message);
  }

  // link catchup to stream parts array
  const unique_ptr_bson_t query_main_stream(BCON_NEW("_id", BCON_OID(&bsid)));
  const unique_ptr_bson_t update_main_stream(BCON_NEW("$push", "{", STREAM_PARTS_FIELD, BCON_OID(&catchupid), "}"));
  if (!mongoc_collection_update(streams_, MONGOC_UPDATE_NONE, stream_query.get(), update_main_stream.get(), NULL,
                                &error)) {
    DEBUG_LOG() << "Failed to add stream to parts stream array: " << error.message;
    return common::make_error(error.message);
  }

  const bson_oid_t* server_oid = bson_iter_oid(&server_id);
  common::Error err = AddStreamToServer(servers_, server_oid, &catchupid);
  if (err) {
    DEBUG_LOG() << "Failed to add stream to server array: " << err->GetDescription();
    return err;
  }

  *serverid = common::ConvertToString(server_oid);
  epg.SetUrls(true_catchups_urls);
  copy.SetEpg(epg);
  copy.SetStreamID(cid_str);
  *is_created = true;
  *cat = copy;
  return common::Error();
}

}  // namespace mongo
}  // namespace server
}  // namespace fastocloud
