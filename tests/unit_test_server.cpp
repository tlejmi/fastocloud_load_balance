#include <daemon/commands_info/prepare_info.h>
#include <gtest/gtest.h>

TEST(PrepareInfo, test) {
  common::net::HostAndPort hs("some", 1234);
  common::file_system::ascii_directory_string_path http_root("/home/sasha/123");

  fastocloud::server::service::PrepareInfo prep(hs, http_root);
  ASSERT_EQ(prep.GetCatchupsHttpRoot(), http_root);
  ASSERT_EQ(prep.GetCatchupsHost(), hs);

  fastocloud::server::service::PrepareInfo::serialize_type ser;
  common::Error err = prep.Serialize(&ser);
  ASSERT_TRUE(!err);

  fastocloud::server::service::PrepareInfo dser;
  err = dser.DeSerialize(ser);
  ASSERT_TRUE(!err);

  ASSERT_EQ(prep, dser);
}
