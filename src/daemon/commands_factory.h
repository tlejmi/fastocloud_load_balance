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

#include <fastotv/protocol/types.h>

#include <common/daemon/commands/activate_info.h>
#include <common/daemon/commands/ping_info.h>
#include <common/daemon/commands/stop_info.h>

namespace fastocloud {
namespace server {

// requests
common::Error StopServiceRequest(fastotv::protocol::sequance_id_t id,
                                 const common::daemon::commands::StopInfo& params,
                                 fastotv::protocol::request_t* req);
common::Error PingRequest(fastotv::protocol::sequance_id_t id,
                          const common::daemon::commands::ClientPingInfo& params,
                          fastotv::protocol::request_t* req);

// responses service
common::Error StopServiceResponseSuccess(fastotv::protocol::sequance_id_t id, fastotv::protocol::response_t* resp);
common::Error StopServiceResponseFail(fastotv::protocol::sequance_id_t id,
                                      const std::string& error_text,
                                      fastotv::protocol::response_t* resp);

common::Error PingServiceResponce(fastotv::protocol::sequance_id_t id,
                                  const common::daemon::commands::ServerPingInfo& ping,
                                  fastotv::protocol::response_t* resp);
common::Error PingServiceResponceFail(fastotv::protocol::sequance_id_t id,
                                      const std::string& error_text,
                                      fastotv::protocol::response_t* resp);

common::Error ActivateResponse(fastotv::protocol::sequance_id_t id,
                               const std::string& result,
                               fastotv::protocol::response_t* resp);
common::Error ActivateResponseFail(fastotv::protocol::sequance_id_t id,
                                   const std::string& error_text,
                                   fastotv::protocol::response_t* resp);

common::Error PrepareServiceResponceSuccess(fastotv::protocol::sequance_id_t id, fastotv::protocol::response_t* resp);
common::Error GetLogServiceResponseSuccess(fastotv::protocol::sequance_id_t id, fastotv::protocol::response_t* resp);
}  // namespace server
}  // namespace fastocloud
