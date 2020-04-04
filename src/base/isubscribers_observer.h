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

#include "base/server_auth_info.h"

namespace fastocloud {
namespace server {
namespace base {

class ISubscribersObserver {
 public:
  virtual void OnSubscriberConnected(const ServerDBAuthInfo& info) = 0;
  virtual void OnSubscriberDisConnected(const ServerDBAuthInfo& info) = 0;
};

}  // namespace base
}  // namespace server
}  // namespace fastocloud
