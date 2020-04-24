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

#include "mongo/mongo2info.h"

#include <string>
#include <vector>

namespace fastocloud {
namespace server {
namespace mongo {

namespace details {
std::vector<common::uri::Url> MakeUrlsFromOutput(const std::vector<fastotv::OutputUri>& output) {
  std::vector<common::uri::Url> result;
  for (size_t i = 0; i < output.size(); ++i) {
    result.push_back(output[i].GetOutput());
  }
  return result;
}
}  // namespace details

bool MakeVodInfo(const bson_t* sdoc,
                 fastotv::StreamType st,
                 const UserStreamInfo& uinfo,
                 fastotv::commands_info::VodInfo* cinf) {
  if (!sdoc || !cinf) {
    return false;
  }

  bson_iter_t iter;
  if (!bson_iter_init(&iter, sdoc)) {
    return false;
  }

#define CHECK_SUM_VOD 16

  std::string sid_str;
  std::string group;
  int iarc;
  int view_count;
  fastotv::commands_info::MovieInfo mov;
  fastotv::commands_info::StreamBaseInfo::parts_t parts;
  fastotv::commands_info::StreamBaseInfo::groups_t groups;
  int check_sum = 0;
  bool have_audio = true;
  bool have_video = true;
  while (bson_iter_next(&iter) && check_sum != CHECK_SUM_VOD) {
    const char* key = bson_iter_key(&iter);
    if (strcmp(key, STREAM_ID_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_OID(&iter)) {
        return false;
      }
      const bson_oid_t* oid = bson_iter_oid(&iter);
      sid_str = common::ConvertToString(oid);
      check_sum++;
    } else if (strcmp(key, STREAM_GROUPS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }
      uint32_t array_len;
      const uint8_t* array = nullptr;
      bson_iter_array(&iter, &array_len, &array);
      bson_t array_doc;
      if (bson_init_static(&array_doc, array, array_len)) {
        bson_iter_t it;
        if (bson_iter_init(&it, &array_doc)) {
          while (bson_iter_next(&iter)) {
            std::string group = bson_iter_utf8(&it, NULL);
            groups.push_back(group);
          }
        }
      }

      check_sum++;
    } else if (strcmp(key, STREAM_IARC_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      iarc = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_VIEW_COUNT_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      view_count = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_NAME_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      mov.SetDisplayName(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, VOD_DESCRIPTION_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      mov.SetDescription(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, VOD_PRVIEW_ICON_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      mov.SetPreviewIcon(common::uri::Url(bson_iter_utf8(&iter, NULL)));
      check_sum++;
    } else if (strcmp(key, STREAM_OUTPUT_FIELD) == 0) {
      std::vector<fastotv::OutputUri> urls;
      if (!GetOutputUrlData(&iter, &urls)) {
        return false;
      }

      mov.SetUrls(details::MakeUrlsFromOutput(urls));
      check_sum++;
    } else if (strcmp(key, STREAM_HAVE_AUDIO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_audio = bson_iter_bool(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_PARTS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }

      bson_iter_t bparts;
      if (!bson_iter_recurse(&iter, &bparts)) {
        return false;
      }

      while (bson_iter_next(&bparts)) {
        const bson_oid_t* oid = bson_iter_oid(&bparts);
        std::string part_str = common::ConvertToString(oid);
        parts.push_back(part_str);
      }
    } else if (strcmp(key, STREAM_HAVE_VIDEO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_video = bson_iter_bool(&iter);
      check_sum++;
    } else if (strcmp(key, VOD_TRAILER_URL_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      mov.SetTrailerUrl(common::uri::Url(bson_iter_utf8(&iter, NULL)));
      check_sum++;
    } else if (strcmp(key, VOD_USER_SCORE_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_DOUBLE(&iter)) {
        return false;
      }
      mov.SetUserScore(bson_iter_double(&iter));
      check_sum++;
    } else if (strcmp(key, VOD_PRIME_DATE_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_DATE_TIME(&iter)) {
        return false;
      }
      mov.SetPrimeDate(bson_iter_date_time(&iter));
      check_sum++;
    } else if (strcmp(key, VOD_COUNTRY_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      mov.SetCountry(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, VOD_DURATION_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      mov.SetDuration(bson_iter_int32(&iter));
      check_sum++;
    } else if (strcmp(key, VOD_TYPE_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      mov.SetType(static_cast<fastotv::commands_info::MovieInfo::Type>(bson_iter_int32(&iter)));
      check_sum++;
    }
  }

  if (check_sum != CHECK_SUM_VOD) {
    bool is_proxy_valid = st == fastotv::VOD_PROXY && check_sum == CHECK_SUM_VOD - 2;
    if (!is_proxy_valid) {
      WARNING_LOG() << "Skipped type: " << st << ", check_sum: " << check_sum << ", id: " << sid_str;
      return false;
    }
  }

  *cinf = fastotv::commands_info::VodInfo(sid_str, groups, iarc, uinfo.favorite, uinfo.recent, uinfo.interruption_time,
                                          mov, have_video, have_audio, parts, view_count, uinfo.locked);
  return true;
}  // namespace mongo

bool MakeCatchupInfo(const bson_t* sdoc,
                     fastotv::StreamType st,
                     const UserStreamInfo& uinfo,
                     fastotv::commands_info::CatchupInfo* cinf) {
  if (!sdoc || !cinf) {
    return false;
  }

  bson_iter_t iter;
  if (!bson_iter_init(&iter, sdoc)) {
    return false;
  }

#define CHECK_SUM_CATCHUP 12

  std::string sid_str;
  std::string group;
  fastotv::commands_info::EpgInfo epg;
  fastotv::commands_info::StreamBaseInfo::parts_t parts;
  fastotv::commands_info::StreamBaseInfo::groups_t groups;
  fastotv::timestamp_t start;
  fastotv::timestamp_t stop;
  int check_sum = 0;
  int iarc;
  int view_count;
  bool have_audio = true;
  bool have_video = true;
  while (bson_iter_next(&iter) && check_sum != CHECK_SUM_CATCHUP) {
    const char* key = bson_iter_key(&iter);
    if (strcmp(key, STREAM_ID_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_OID(&iter)) {
        return false;
      }
      const bson_oid_t* oid = bson_iter_oid(&iter);
      sid_str = common::ConvertToString(oid);
      check_sum++;
    } else if (strcmp(key, STREAM_GROUPS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }
      uint32_t array_len;
      const uint8_t* array = nullptr;
      bson_iter_array(&iter, &array_len, &array);
      bson_t array_doc;
      if (bson_init_static(&array_doc, array, array_len)) {
        bson_iter_t it;
        if (bson_iter_init(&it, &array_doc)) {
          while (bson_iter_next(&iter)) {
            std::string group = bson_iter_utf8(&it, NULL);
            groups.push_back(group);
          }
        }
      }
      check_sum++;
    } else if (strcmp(key, STREAM_IARC_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      iarc = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_VIEW_COUNT_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      view_count = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, CHANNEL_TVG_ID_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetTvgID(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, STREAM_NAME_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetDisplayName(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, STREAM_HAVE_AUDIO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_audio = bson_iter_bool(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_PARTS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }

      bson_iter_t bparts;
      if (!bson_iter_recurse(&iter, &bparts)) {
        return false;
      }

      while (bson_iter_next(&bparts)) {
        const bson_oid_t* oid = bson_iter_oid(&bparts);
        std::string part_str = common::ConvertToString(oid);
        parts.push_back(part_str);
      }
    } else if (strcmp(key, STREAM_HAVE_VIDEO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_video = bson_iter_bool(&iter);
      check_sum++;

    } else if (strcmp(key, CATCHUP_START_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_DATE_TIME(&iter)) {
        return false;
      }
      start = bson_iter_date_time(&iter);
      check_sum++;
    } else if (strcmp(key, CATCHUP_STOP_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_DATE_TIME(&iter)) {
        return false;
      }
      stop = bson_iter_date_time(&iter);
      check_sum++;
    } else if (strcmp(key, CHANNEL_TVG_LOGO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetIconUrl(common::uri::Url(bson_iter_utf8(&iter, NULL)));
      check_sum++;
    } else if (strcmp(key, STREAM_OUTPUT_FIELD) == 0) {
      std::vector<fastotv::OutputUri> urls;
      if (!GetOutputUrlData(&iter, &urls)) {
        return false;
      }
      epg.SetUrls(details::MakeUrlsFromOutput(urls));
      check_sum++;
    }
  }

  if (check_sum != CHECK_SUM_CATCHUP) {
    WARNING_LOG() << "Skipped type: " << st << ", check_sum: " << check_sum << ", id: " << sid_str;
    return false;
  }

  *cinf =
      fastotv::commands_info::CatchupInfo(sid_str, groups, iarc, uinfo.favorite, uinfo.recent, uinfo.interruption_time,
                                          epg, have_video, have_audio, parts, view_count, uinfo.locked, start, stop);
  return true;
}

bool MakeChannelInfo(const bson_t* sdoc,
                     fastotv::StreamType st,
                     const UserStreamInfo& uinfo,
                     fastotv::commands_info::ChannelInfo* cinf) {
  if (!sdoc || !cinf) {
    return false;
  }

  bson_iter_t iter;
  if (!bson_iter_init(&iter, sdoc)) {
    return false;
  }

#define CHECK_SUM_CHANNEL 10

  std::string sid_str;
  std::string group;
  fastotv::commands_info::EpgInfo epg;
  fastotv::commands_info::StreamBaseInfo::parts_t parts;
  fastotv::commands_info::StreamBaseInfo::groups_t groups;
  int check_sum = 0;
  int iarc;
  int view_count;
  bool have_audio = true;
  bool have_video = true;
  while (bson_iter_next(&iter) && check_sum != CHECK_SUM_CHANNEL) {
    const char* key = bson_iter_key(&iter);
    if (strcmp(key, STREAM_ID_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_OID(&iter)) {
        return false;
      }
      const bson_oid_t* oid = bson_iter_oid(&iter);
      sid_str = common::ConvertToString(oid);
      check_sum++;
    } else if (strcmp(key, STREAM_GROUPS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }
      uint32_t array_len;
      const uint8_t* array = nullptr;
      bson_iter_array(&iter, &array_len, &array);
      bson_t array_doc;
      if (bson_init_static(&array_doc, array, array_len)) {
        bson_iter_t it;
        if (bson_iter_init(&it, &array_doc)) {
          while (bson_iter_next(&iter)) {
            std::string group = bson_iter_utf8(&it, NULL);
            groups.push_back(group);
          }
        }
      }
      check_sum++;
    } else if (strcmp(key, STREAM_IARC_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      iarc = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_VIEW_COUNT_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_INT32(&iter)) {
        return false;
      }
      view_count = bson_iter_int32(&iter);
      check_sum++;
    } else if (strcmp(key, CHANNEL_TVG_ID_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetTvgID(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, STREAM_NAME_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetDisplayName(bson_iter_utf8(&iter, NULL));
      check_sum++;
    } else if (strcmp(key, STREAM_HAVE_AUDIO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_audio = bson_iter_bool(&iter);
      check_sum++;
    } else if (strcmp(key, STREAM_PARTS_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_ARRAY(&iter)) {
        return false;
      }

      bson_iter_t bparts;
      if (!bson_iter_recurse(&iter, &bparts)) {
        return false;
      }

      while (bson_iter_next(&bparts)) {
        const bson_oid_t* oid = bson_iter_oid(&bparts);
        std::string part_str = common::ConvertToString(oid);
        parts.push_back(part_str);
      }
    } else if (strcmp(key, STREAM_HAVE_VIDEO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_BOOL(&iter)) {
        return false;
      }
      have_video = bson_iter_bool(&iter);
      check_sum++;
    } else if (strcmp(key, CHANNEL_TVG_LOGO_FIELD) == 0) {
      if (!BSON_ITER_HOLDS_UTF8(&iter)) {
        return false;
      }
      epg.SetIconUrl(common::uri::Url(bson_iter_utf8(&iter, NULL)));
      check_sum++;
    } else if (strcmp(key, STREAM_OUTPUT_FIELD) == 0) {
      std::vector<fastotv::OutputUri> urls;
      if (!GetOutputUrlData(&iter, &urls)) {
        return false;
      }
      epg.SetUrls(details::MakeUrlsFromOutput(urls));
      check_sum++;
    }
  }

  if (check_sum != CHECK_SUM_CHANNEL) {
    bool is_proxy_valid = st == fastotv::PROXY && check_sum == CHECK_SUM_CHANNEL - 2;
    if (!is_proxy_valid) {
      WARNING_LOG() << "Skipped type: " << st << ", check_sum: " << check_sum << ", id: " << sid_str;
      return false;
    }
  }

  *cinf =
      fastotv::commands_info::ChannelInfo(sid_str, groups, iarc, uinfo.favorite, uinfo.recent, uinfo.interruption_time,
                                          epg, have_video, have_audio, parts, view_count, uinfo.locked);
  return true;
}

bool GetHttpRootFromStream(const bson_t* sdoc,
                           fastotv::StreamType st,
                           fastotv::channel_id_t cid,
                           common::file_system::ascii_directory_string_path* dir) {
  UNUSED(st);
  if (!sdoc || !dir) {
    return false;
  }

  bson_iter_t iter;
  if (!bson_iter_init(&iter, sdoc)) {
    return false;
  }

  while (bson_iter_next(&iter)) {
    const char* key = bson_iter_key(&iter);
    if (strcmp(key, STREAM_OUTPUT_FIELD) == 0) {
      std::vector<fastotv::OutputUri> urls;
      if (!GetOutputUrlData(&iter, &urls)) {
        return false;
      }

      for (size_t i = 0; i < urls.size(); ++i) {
        if (urls[i].GetID() == cid) {
          const auto http_root = urls[i].GetHttpRoot();
          if (!http_root) {
            return false;
          }
          *dir = *http_root;
          return true;
        }
      }
      return false;
    }
  }

  return false;
}

bool GetUrlFromStream(const bson_t* sdoc, fastotv::StreamType st, fastotv::channel_id_t cid, common::uri::Url* url) {
  UNUSED(st);
  if (!sdoc || !url) {
    return false;
  }

  bson_iter_t iter;
  if (!bson_iter_init(&iter, sdoc)) {
    return false;
  }

  while (bson_iter_next(&iter)) {
    const char* key = bson_iter_key(&iter);
    if (strcmp(key, STREAM_OUTPUT_FIELD) == 0) {
      std::vector<fastotv::OutputUri> urls;
      if (!GetOutputUrlData(&iter, &urls)) {
        return false;
      }

      for (size_t i = 0; i < urls.size(); ++i) {
        if (urls[i].GetID() == cid) {
          *url = urls[i].GetOutput();
          return true;
        }
      }
      return false;
    }
  }

  return false;
}

bool GetOutputUrlData(bson_iter_t* iter, std::vector<fastotv::OutputUri>* urls) {
  if (!iter || !urls) {
    return false;
  }

  if (!BSON_ITER_HOLDS_ARRAY(iter)) {
    return false;
  }

  bson_iter_t ar;
  if (!bson_iter_recurse(iter, &ar)) {
    return false;
  }

  std::vector<fastotv::OutputUri> lurls;
  while (bson_iter_next(&ar)) {
    bson_iter_t bid;
    if (bson_iter_recurse(&ar, &bid) && bson_iter_find(&bid, STREAM_OUTPUT_URLS_ID_FIELD) &&
        BSON_ITER_HOLDS_INT32(&bid)) {
      fastotv::channel_id_t lcid = bson_iter_int32(&bid);
      bson_iter_t burl;
      if (bson_iter_recurse(&ar, &burl) && bson_iter_find(&burl, STREAM_OUTPUT_URLS_URI_FIELD) &&
          BSON_ITER_HOLDS_UTF8(&burl)) {
        common::uri::Url uri(bson_iter_utf8(&burl, NULL));
        fastotv::OutputUri out(lcid, uri);
        bson_iter_t bhttp_root;
        if (bson_iter_recurse(&ar, &bhttp_root) && bson_iter_find(&bhttp_root, STREAM_OUTPUT_URLS_HTTP_ROOT_FIELD) &&
            BSON_ITER_HOLDS_UTF8(&bhttp_root)) {
          common::file_system::ascii_directory_string_path http_root =
              common::file_system::ascii_directory_string_path(bson_iter_utf8(&bhttp_root, NULL));
          out.SetHttpRoot(http_root);
        }
        bson_iter_t bhls_type;
        if (bson_iter_recurse(&ar, &bhls_type) && bson_iter_find(&bhls_type, STREAM_OUTPUT_URLS_HLS_TYPE_FIELD) &&
            BSON_ITER_HOLDS_INT32(&bhls_type)) {
          out.SetHlsType(static_cast<fastotv::OutputUri::HlsType>(bson_iter_int32(&bhls_type)));
        }
        lurls.push_back(out);
      }
    }
  }

  *urls = lurls;
  return true;
}

}  // namespace mongo
}  // namespace server
}  // namespace fastocloud

namespace common {
std::string ConvertToString(const bson_oid_t* oid) {
  char data[25] = {0};
  bson_oid_to_string(oid, data);
  return data;
}

bool ConvertFromString(const std::string& soid, bson_oid_t* out) {
  if (soid.empty() || !out) {
    return false;
  }

  const char* oid_ptr = soid.c_str();
  bson_oid_t result;
  bson_oid_init_from_string(&result, oid_ptr);
  *out = result;
  return true;
}
}  // namespace common
