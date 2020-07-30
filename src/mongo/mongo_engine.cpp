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

#include "mongo/mongo_engine.h"

namespace fastocloud {
namespace server {
namespace mongo {

common::ErrnoError MongoEngine::Connect(const std::string& url, mongoc_client_t** connection) {
  if (url.empty() || !connection) {
    return common::make_errno_error_inval();
  }

  mongoc_uri_t* uri = mongoc_uri_new(url.c_str());
  if (!uri) {
    return common::make_errno_error("Invalid uri", EINVAL);
  }

  mongoc_client_t* cl = mongoc_client_new_from_uri(uri);
  mongoc_uri_destroy(uri);

  bson_t reply;
  bson_error_t error;
  if (!mongoc_client_get_server_status(cl, nullptr, &reply, &error)) {
    mongoc_client_destroy(cl);
    return common::make_errno_error(error.message, EAGAIN);
  }

  bson_destroy(&reply);
  *connection = cl;
  return common::ErrnoError();
}

MongoEngine::MongoEngine() {
  mongoc_init();
}

MongoEngine::~MongoEngine() {
  mongoc_cleanup();
}

void MongoQueryDeleter::operator()(bson_t* bson) const {
  if (bson) {
    bson_destroy(bson);
  }
}

void MongoCursorDeleter::operator()(mongoc_cursor_t* cursor) const {
  if (cursor) {
    mongoc_cursor_destroy(cursor);
  }
}

}  // namespace mongo
}  // namespace server
}  // namespace fastocloud
