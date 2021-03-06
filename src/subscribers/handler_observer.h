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

#include <fastotv/commands_info/catchup_info.h>
#include <fastotv/commands_info/content_request_info.h>

namespace fastocloud {
namespace server {
namespace subscribers {

class SubscribersHandler;

class ISubscribersHandlerObserver {
 public:
  virtual void CatchupCreated(SubscribersHandler* handler,
                              std::string serverid,
                              const fastotv::commands_info::CatchupInfo& chan) = 0;
  virtual void ContentRequestCreated(SubscribersHandler* handler,
                                     const fastotv::commands_info::ContentRequestInfo& request) = 0;
};

}  // namespace subscribers
}  // namespace server
}  // namespace fastocloud
