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

#include "base/isubscribers_manager.h"

#include "base/isubscribers_observer.h"

namespace fastocloud {
namespace server {
namespace base {

bool CatchupEndpointInfo::IsValid() const {
  return catchups_host.IsValid() && catchups_http_root.IsValid();
}

ISubscribersManager::ISubscribersManager(ISubscribersObserver* observer) : observer_(observer) {}

common::Error ISubscribersManager::RegisterInnerConnectionByHost(SubscriberInfo* client, const ServerDBAuthInfo& info) {
  UNUSED(info);
  if (observer_) {
    if (client) {
      const auto login = client->GetLogin();
      if (login) {
        observer_->OnSubscriberConnected(*login);
      }
    }
  }
  return common::Error();
}

common::Error ISubscribersManager::UnRegisterInnerConnectionByHost(SubscriberInfo* client) {
  if (observer_) {
    if (client) {
      const auto login = client->GetLogin();
      if (login) {
        observer_->OnSubscriberDisConnected(*login);
      }
    }
  }
  return common::Error();
}

ISubscribersManager::~ISubscribersManager() {}

}  // namespace base
}  // namespace server
}  // namespace fastocloud
