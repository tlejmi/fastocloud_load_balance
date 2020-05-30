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

#include "process_slave_wrapper.h"

#include <thread>
#include <vector>

#include <common/daemon/commands/activate_info.h>
#include <common/daemon/commands/get_log_info.h>
#include <common/daemon/commands/stop_info.h>
#include <common/license/expire_license.h>
#include <common/net/http_client.h>
#include <common/net/net.h>

#include "daemon/client.h"
#include "daemon/commands.h"
#include "daemon/commands_info/details/shots.h"
#include "daemon/commands_info/notify_subscriber_info.h"
#include "daemon/commands_info/prepare_info.h"
#include "daemon/commands_info/server_info.h"
#include "daemon/commands_info/sync_info.h"
#include "daemon/server.h"

#include "http/handler.h"
#include "http/server.h"

#include "mongo/subscribers_manager.h"

#include "subscribers/handler.h"
#include "subscribers/server.h"

namespace fastocloud {
namespace server {

struct ProcessSlaveWrapper::NodeStats {
  NodeStats() : prev(), prev_nshot(), timestamp(common::time::current_utc_mstime()) {}

  service::CpuShot prev;
  service::NetShot prev_nshot;
  fastotv::timestamp_t timestamp;
};

ProcessSlaveWrapper::ProcessSlaveWrapper(const Config& config)
    : config_(config),
      loop_(nullptr),
      subscribers_server_(nullptr),
      subscribers_handler_(nullptr),
      http_server_(nullptr),
      http_handler_(nullptr),
      ping_client_timer_(INVALID_TIMER_ID),
      node_stats_timer_(INVALID_TIMER_ID),
      check_license_timer_(INVALID_TIMER_ID),
      node_stats_(new NodeStats) {
  loop_ = new DaemonServer(config.host, this);
  loop_->SetName("client_server");

  mongo::SubscribersManager* sub_manager = new mongo::SubscribersManager(this);
  ignore_result(sub_manager->ConnectToDatabase(config.mongodb_url));
  sub_manager_ = sub_manager;

  subscribers_handler_ =
      new subscribers::SubscribersHandler(this, sub_manager_, config.epg_url, config.locked_stream_text);
  subscribers_server_ = new subscribers::SubscribersServer(config.subscribers_host, subscribers_handler_);
  subscribers_server_->SetName("subscribers_server");

  http_handler_ = new http::HttpHandler(sub_manager_);
  http_server_ = new http::HttpServer(config.http_host, http_handler_);
  http_server_->SetName("http_server");
}

common::ErrnoError ProcessSlaveWrapper::SendStopDaemonRequest(const Config& config) {
  if (!config.IsValid()) {
    return common::make_errno_error_inval();
  }

  common::net::HostAndPort host = config.host;
  if (host.GetHost() == PROJECT_NAME_LOWERCASE) {  // docker image
    host = common::net::HostAndPort::CreateLocalHost(host.GetPort());
  }

  common::net::socket_info client_info;
  common::ErrnoError err = common::net::connect(host, common::net::ST_SOCK_STREAM, nullptr, &client_info);
  if (err) {
    return err;
  }

  std::unique_ptr<ProtocoledDaemonClient> connection(new ProtocoledDaemonClient(nullptr, client_info));
  err = connection->StopMe();
  if (err) {
    ignore_result(connection->Close());
    return err;
  }

  ignore_result(connection->Close());
  return common::ErrnoError();
}

ProcessSlaveWrapper::~ProcessSlaveWrapper() {
  ignore_result((static_cast<mongo::SubscribersManager*>(sub_manager_))->Disconnect());
  destroy(&http_server_);
  destroy(&http_handler_);
  destroy(&subscribers_server_);
  destroy(&subscribers_handler_);
  destroy(&sub_manager_);
  destroy(&loop_);
  destroy(&node_stats_);
}

int ProcessSlaveWrapper::Exec() {
  subscribers::SubscribersServer* subs_server = static_cast<subscribers::SubscribersServer*>(subscribers_server_);
  std::thread subs_thread = std::thread([subs_server] {
    common::ErrnoError err = subs_server->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    err = subs_server->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    int res = subs_server->Exec();
    UNUSED(res);
  });

  http::HttpServer* http_server = static_cast<http::HttpServer*>(http_server_);
  std::thread http_thread = std::thread([http_server] {
    common::ErrnoError err = http_server->Bind(true);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    err = http_server->Listen(5);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      return;
    }

    int res = http_server->Exec();
    UNUSED(res);
  });

  int res = EXIT_FAILURE;
  DaemonServer* server = static_cast<DaemonServer*>(loop_);
  common::ErrnoError err = server->Bind(true);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  err = server->Listen(5);
  if (err) {
    DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    goto finished;
  }

  node_stats_->prev = service::GetMachineCpuShot();
  node_stats_->prev_nshot = service::GetMachineNetShot();
  node_stats_->timestamp = common::time::current_utc_mstime();
  res = server->Exec();

finished:
  subs_thread.join();
  http_thread.join();
  return res;
}

void ProcessSlaveWrapper::OnSubscriberConnected(const base::FrontSubscriberInfo& info) {
  fastotv::protocol::request_t req;
  common::Error err_ser = SubscriberConnectedBroadcast(info, &req);
  if (err_ser) {
    return;
  }

  loop_->ExecInLoopThread([this, req]() { BroadcastClients(req); });
  INFO_LOG() << "Welcome: " << info.GetLogin();
}

void ProcessSlaveWrapper::OnSubscriberDisConnected(const base::FrontSubscriberInfo& info) {
  fastotv::protocol::request_t req;
  common::Error err_ser = SubscriberDisConnectedBroadcast(info, &req);
  if (err_ser) {
    return;
  }

  loop_->ExecInLoopThread([this, req]() { BroadcastClients(req); });
  INFO_LOG() << "Bye: " << info.GetLogin();
}

void ProcessSlaveWrapper::PreLooped(common::libev::IoLoop* server) {
  ping_client_timer_ = server->CreateTimer(ping_timeout_clients_seconds, true);
  node_stats_timer_ = server->CreateTimer(node_stats_send_seconds, true);
  check_license_timer_ = server->CreateTimer(check_license_timeout_seconds, true);
}

void ProcessSlaveWrapper::Accepted(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoClient* client) {
  UNUSED(server);
  UNUSED(client);
}

void ProcessSlaveWrapper::Closed(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::TimerEmited(common::libev::IoLoop* server, common::libev::timer_id_t id) {
  if (ping_client_timer_ == id) {
    std::vector<common::libev::IoClient*> online_clients = server->GetClients();
    for (size_t i = 0; i < online_clients.size(); ++i) {
      common::libev::IoClient* client = online_clients[i];
      ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client);
      if (dclient && dclient->IsVerified()) {
        common::ErrnoError err = dclient->Ping();
        if (err) {
          DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
          ignore_result(dclient->Close());
          delete dclient;
        } else {
          INFO_LOG() << "Sent ping to client[" << client->GetFormatedName() << "], from server["
                     << server->GetFormatedName() << "], " << online_clients.size() << " client(s) connected.";
        }
      }
    }
  } else if (node_stats_timer_ == id) {
    const std::string node_stats = MakeServiceStats(0);
    fastotv::protocol::request_t req;
    common::Error err_ser = StatisitcServiceBroadcast(node_stats, &req);
    if (err_ser) {
      return;
    }

    BroadcastClients(req);
  } else if (check_license_timer_ == id) {
    CheckLicenseExpired();
  }
}

void ProcessSlaveWrapper::Accepted(common::libev::IoChild* child) {
  UNUSED(child);
}

void ProcessSlaveWrapper::Moved(common::libev::IoLoop* server, common::libev::IoChild* child) {
  UNUSED(server);
  UNUSED(child);
}

void ProcessSlaveWrapper::ChildStatusChanged(common::libev::IoChild* child, int status, int signal) {
  UNUSED(child);
  UNUSED(status);
  UNUSED(signal);
}

void ProcessSlaveWrapper::StopImpl() {
  subscribers_server_->Stop();
  http_server_->Stop();
  loop_->Stop();
}

void ProcessSlaveWrapper::BroadcastClients(const fastotv::protocol::request_t& req) {
  std::vector<common::libev::IoClient*> clients = loop_->GetClients();
  for (size_t i = 0; i < clients.size(); ++i) {
    ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(clients[i]);
    if (dclient && dclient->IsVerified()) {
      common::ErrnoError err = dclient->WriteRequest(req);
      if (err) {
        WARNING_LOG() << "BroadcastClients error: " << err->GetDescription();
      }
    }
  }
}

common::ErrnoError ProcessSlaveWrapper::DaemonDataReceived(ProtocoledDaemonClient* dclient) {
  CHECK(loop_->IsLoopThread());
  std::string input_command;
  common::ErrnoError err = dclient->ReadCommand(&input_command);
  if (err) {
    return err;  // i don't want handle spam, comand must be foramated according protocol
  }

  fastotv::protocol::request_t* req = nullptr;
  fastotv::protocol::response_t* resp = nullptr;
  common::Error err_parse = common::protocols::json_rpc::ParseJsonRPC(input_command, &req, &resp);
  if (err_parse) {
    const std::string err_str = err_parse->GetDescription();
    return common::make_errno_error(err_str, EAGAIN);
  }

  if (req) {
    DEBUG_LOG() << "Received daemon request: " << input_command;
    err = HandleRequestServiceCommand(dclient, req);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete req;
  } else if (resp) {
    DEBUG_LOG() << "Received daemon responce: " << input_command;
    err = HandleResponceServiceCommand(dclient, resp);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
    }
    delete resp;
  } else {
    DNOTREACHED();
    return common::make_errno_error("Invalid command type.", EINVAL);
  }

  return common::ErrnoError();
}

void ProcessSlaveWrapper::DataReceived(common::libev::IoClient* client) {
  if (ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(client)) {
    common::ErrnoError err = DaemonDataReceived(dclient);
    if (err) {
      DEBUG_MSG_ERROR(err, common::logging::LOG_LEVEL_ERR);
      ignore_result(dclient->Close());
      delete dclient;
    }
  } else {
    NOTREACHED();
  }
}

void ProcessSlaveWrapper::DataReadyToWrite(common::libev::IoClient* client) {
  UNUSED(client);
}

void ProcessSlaveWrapper::PostLooped(common::libev::IoLoop* server) {
  if (ping_client_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(ping_client_timer_);
    ping_client_timer_ = INVALID_TIMER_ID;
  }
  if (node_stats_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(node_stats_timer_);
    node_stats_timer_ = INVALID_TIMER_ID;
  }
  if (check_license_timer_ != INVALID_TIMER_ID) {
    server->RemoveTimer(check_license_timer_);
    check_license_timer_ = INVALID_TIMER_ID;
  }
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientStopService(ProtocoledDaemonClient* dclient,
                                                                       const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    const auto info = dclient->GetInfo();
    common::net::HostAndPort host(info.host(), info.port());
    INFO_LOG() << "Stop request from host: " << common::ConvertToString(host);
    if (!host.IsLocalHost()) {
      return common::make_errno_error_inval();
    }
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    common::daemon::commands::StopInfo stop_info;
    common::Error err_des = stop_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::ErrnoError err = dclient->StopSuccess(req->id);
    StopImpl();
    return err;
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientGetLogService(ProtocoledDaemonClient* dclient,
                                                                         const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jlog = json_tokener_parse(params_ptr);
    if (!jlog) {
      return common::make_errno_error_inval();
    }

    common::daemon::commands::GetLogInfo get_log_info;
    common::Error err_des = get_log_info.DeSerialize(jlog);
    json_object_put(jlog);
    if (err_des) {
      ignore_result(dclient->GetLogServiceFail(req->id, err_des));
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    const auto remote_log_path = get_log_info.GetLogPath();
    if (!remote_log_path.SchemeIsHTTPOrHTTPS()) {
      common::ErrnoError errn = common::make_errno_error("Not supported protocol", EAGAIN);
      ignore_result(dclient->GetLogServiceFail(req->id, common::make_error_from_errno(errn)));
      return errn;
    }
    common::Error err =
        common::net::PostHttpFile(common::file_system::ascii_file_string_path(config_.log_path), remote_log_path);
    if (err) {
      ignore_result(dclient->GetLogServiceFail(req->id, err));
      const std::string err_str = err->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    return dclient->GetLogServiceSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientSendMessageForSubscriber(
    ProtocoledDaemonClient* dclient,
    const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jnotify = json_tokener_parse(params_ptr);
    if (!jnotify) {
      return common::make_errno_error_inval();
    }

    service::NotifySubscriberInfo notify;
    common::Error err_des = notify.DeSerialize(jnotify);
    json_object_put(jnotify);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    common::Error err = sub_manager_->SendSubscriberNotification(notify.GetUserID(), notify.GetDeviceID(), notify);
    if (err) {
      ignore_result(dclient->SendSubscriberMessageFail(req->id, err));
      return common::make_errno_error(err->GetDescription(), EAGAIN);
    }
    return dclient->SendSubscriberMessageSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPrepareService(ProtocoledDaemonClient* dclient,
                                                                          const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jservice_state = json_tokener_parse(params_ptr);
    if (!jservice_state) {
      return common::make_errno_error_inval();
    }

    service::PrepareInfo state_info;
    common::Error err_des = state_info.DeSerialize(jservice_state);
    json_object_put(jservice_state);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    sub_manager_->SetupCatchupsEndpoint({state_info.GetCatchupsHost(), state_info.GetCatchupsHttpRoot()});

    service::StateInfo state;
    state.SetOnlineClients(sub_manager_->GetOnlineSubscribers());
    return dclient->PrepareServiceSuccess(req->id, state);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientSyncService(ProtocoledDaemonClient* dclient,
                                                                       const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jservice_state = json_tokener_parse(params_ptr);
    if (!jservice_state) {
      return common::make_errno_error_inval();
    }

    service::SyncInfo sync_info;
    common::Error err_des = sync_info.DeSerialize(jservice_state);
    json_object_put(jservice_state);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    return dclient->SyncServiceSuccess(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientActivate(ProtocoledDaemonClient* dclient,
                                                                    const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jactivate = json_tokener_parse(params_ptr);
    if (!jactivate) {
      return common::make_errno_error_inval();
    }

    common::daemon::commands::ActivateInfo activate_info;
    common::Error err_des = activate_info.DeSerialize(jactivate);
    json_object_put(jactivate);
    if (err_des) {
      ignore_result(dclient->ActivateFail(req->id, err_des));
      return common::make_errno_error(err_des->GetDescription(), EAGAIN);
    }

    const auto expire_key = activate_info.GetLicense();
    common::time64_t tm;
    bool is_valid = common::license::GetExpireTimeFromKey(PROJECT_NAME_LOWERCASE, *expire_key, &tm);
    if (!is_valid) {
      common::Error err = common::make_error("Invalid expire key");
      ignore_result(dclient->ActivateFail(req->id, err));
      return common::make_errno_error(err->GetDescription(), EINVAL);
    }

    const std::string node_stats = MakeServiceStats(tm);
    common::ErrnoError err_ser = dclient->ActivateSuccess(req->id, node_stats);
    if (err_ser) {
      return err_ser;
    }

    dclient->SetVerified(true, tm);
    return common::ErrnoError();
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponcePingService(ProtocoledDaemonClient* dclient,
                                                                  const fastotv::protocol::response_t* resp) {
  UNUSED(dclient);
  CHECK(loop_->IsLoopThread());
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jclient_ping = json_tokener_parse(params_ptr);
    if (!jclient_ping) {
      return common::make_errno_error_inval();
    }

    common::daemon::commands::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jclient_ping);
    json_object_put(jclient_ping);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }
    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceCatchupCreatedService(ProtocoledDaemonClient* dclient,
                                                                            const fastotv::protocol::response_t* resp) {
  UNUSED(dclient);
  CHECK(loop_->IsLoopThread());
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jcatchup = json_tokener_parse(params_ptr);
    if (!jcatchup) {
      return common::make_errno_error_inval();
    }

    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceSendMessageForSubscriber(
    ProtocoledDaemonClient* dclient,
    const fastotv::protocol::response_t* resp) {
  UNUSED(dclient);
  CHECK(loop_->IsLoopThread());
  if (resp->IsMessage()) {
    const char* params_ptr = resp->message->result.c_str();
    json_object* jnotify = json_tokener_parse(params_ptr);
    if (!jnotify) {
      return common::make_errno_error_inval();
    }

    return common::ErrnoError();
  }
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestClientPingService(ProtocoledDaemonClient* dclient,
                                                                       const fastotv::protocol::request_t* req) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  if (req->params) {
    const char* params_ptr = req->params->c_str();
    json_object* jstop = json_tokener_parse(params_ptr);
    if (!jstop) {
      return common::make_errno_error_inval();
    }

    common::daemon::commands::ClientPingInfo client_ping_info;
    common::Error err_des = client_ping_info.DeSerialize(jstop);
    json_object_put(jstop);
    if (err_des) {
      const std::string err_str = err_des->GetDescription();
      return common::make_errno_error(err_str, EAGAIN);
    }

    return dclient->Pong(req->id);
  }

  return common::make_errno_error_inval();
}

common::ErrnoError ProcessSlaveWrapper::HandleRequestServiceCommand(ProtocoledDaemonClient* dclient,
                                                                    const fastotv::protocol::request_t* req) {
  if (req->method == DAEMON_STOP_SERVICE) {
    return HandleRequestClientStopService(dclient, req);
  } else if (req->method == DAEMON_PING_SERVICE) {
    return HandleRequestClientPingService(dclient, req);
  } else if (req->method == DAEMON_ACTIVATE) {
    return HandleRequestClientActivate(dclient, req);
  } else if (req->method == DAEMON_PREPARE_SERVICE) {
    return HandleRequestClientPrepareService(dclient, req);
  } else if (req->method == DAEMON_SYNC_SERVICE) {
    return HandleRequestClientSyncService(dclient, req);
  } else if (req->method == DAEMON_GET_LOG_SERVICE) {
    return HandleRequestClientGetLogService(dclient, req);
  } else if (req->method == DAEMON_CLIENT_SEND_MESSAGE) {
    return HandleRequestClientSendMessageForSubscriber(dclient, req);
  }

  WARNING_LOG() << "Received unknown method: " << req->method;
  return common::ErrnoError();
}

common::ErrnoError ProcessSlaveWrapper::HandleResponceServiceCommand(ProtocoledDaemonClient* dclient,
                                                                     const fastotv::protocol::response_t* resp) {
  CHECK(loop_->IsLoopThread());
  if (!dclient->IsVerified()) {
    return common::make_errno_error_inval();
  }

  fastotv::protocol::request_t req;
  if (dclient->PopRequestByID(resp->id, &req)) {
    if (req.method == DAEMON_SERVER_PING) {
      ignore_result(HandleResponcePingService(dclient, resp));
    } else if (req.method == DAEMON_SERVER_CATCHUP_CREATED) {
      ignore_result(HandleResponceCatchupCreatedService(dclient, resp));
    } else if (req.method == DAEMON_CLIENT_SEND_MESSAGE) {
      ignore_result(HandleResponceSendMessageForSubscriber(dclient, resp));
    } else {
      WARNING_LOG() << "HandleResponceServiceCommand not handled command: " << req.method;
    }
  }

  return common::ErrnoError();
}

void ProcessSlaveWrapper::CatchupCreated(subscribers::SubscribersHandler* handler,
                                         std::string serverid,
                                         const fastotv::commands_info::CatchupInfo& chan) {
  UNUSED(handler);
  fastotv::protocol::request_t req;
  common::Error err_ser = CatchupCreatedBroadcast(serverid, chan, &req);
  if (err_ser) {
    WARNING_LOG() << "Can't create catchup broadcast request";
    return;
  }

  loop_->ExecInLoopThread([this, req]() { BroadcastClients(req); });
}

void ProcessSlaveWrapper::CheckLicenseExpired() {
  const auto license = config_.license_key;
  if (!license) {
    WARNING_LOG() << "You have an invalid license, service stopped";
    StopImpl();
    return;
  }

  common::time64_t tm;
  bool is_valid = common::license::GetExpireTimeFromKey(PROJECT_NAME_LOWERCASE, *license, &tm);
  if (!is_valid) {
    WARNING_LOG() << "You have an invalid license, service stopped";
    StopImpl();
    return;
  }

  if (tm < common::time::current_utc_mstime()) {
    WARNING_LOG() << "Your license have expired, service stopped";
    StopImpl();
    return;
  }
}

std::string ProcessSlaveWrapper::MakeServiceStats(common::time64_t expiration_time) const {
  service::CpuShot next = service::GetMachineCpuShot();
  double cpu_load = service::GetCpuMachineLoad(node_stats_->prev, next);
  node_stats_->prev = next;

  service::NetShot next_nshot = service::GetMachineNetShot();
  uint64_t bytes_recv = (next_nshot.bytes_recv - node_stats_->prev_nshot.bytes_recv);
  uint64_t bytes_send = (next_nshot.bytes_send - node_stats_->prev_nshot.bytes_send);
  node_stats_->prev_nshot = next_nshot;

  service::MemoryShot mem_shot = service::GetMachineMemoryShot();
  service::HddShot hdd_shot = service::GetMachineHddShot();
  service::SysinfoShot sshot = service::GetMachineSysinfoShot();
  std::string uptime_str = common::MemSPrintf("%lu %lu %lu", sshot.loads[0], sshot.loads[1], sshot.loads[2]);
  fastotv::timestamp_t current_time = common::time::current_utc_mstime();
  fastotv::timestamp_t ts_diff = (current_time - node_stats_->timestamp) / 1000;
  if (ts_diff == 0) {
    ts_diff = 1;  // divide by zero
  }
  node_stats_->timestamp = current_time;

  size_t daemons_client_count = 0;
  std::vector<common::libev::IoClient*> clients = loop_->GetClients();
  for (size_t i = 0; i < clients.size(); ++i) {
    ProtocoledDaemonClient* dclient = dynamic_cast<ProtocoledDaemonClient*>(clients[i]);
    if (dclient && dclient->IsVerified()) {
      daemons_client_count++;
    }
  }
  service::OnlineUsers online(daemons_client_count, static_cast<http::HttpHandler*>(http_handler_)->GetOnlineClients(),
                              static_cast<subscribers::SubscribersHandler*>(subscribers_handler_)->GetOnlineClients());
  service::ServerInfo stat(cpu_load, uptime_str, mem_shot, hdd_shot, bytes_recv / ts_diff, bytes_send / ts_diff, sshot,
                           current_time, online);

  std::string node_stats;
  if (expiration_time != 0) {
    service::FullServiceInfo fstat(config_.http_host, expiration_time, stat);
    common::Error err_ser = fstat.SerializeToString(&node_stats);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      WARNING_LOG() << "Failed to generate node full statistic: " << err_str;
    }
  } else {
    common::Error err_ser = stat.SerializeToString(&node_stats);
    if (err_ser) {
      const std::string err_str = err_ser->GetDescription();
      WARNING_LOG() << "Failed to generate node statistic: " << err_str;
    }
  }
  return node_stats;
}

}  // namespace server
}  // namespace fastocloud
