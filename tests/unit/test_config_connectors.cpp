/**
 * @file tests/unit/test_config_connectors.cpp
 * @brief Connector helpers for dual-KMS capture routing.
 */
#include "../tests_common.h"

#include <src/config.h>

TEST(ConfigConnectors, SysfsEntryParsesCardConnectorSuffix) {
  EXPECT_EQ(config::drm_connector_from_sysfs_entry("card2-DP-1"), "DP-1");
  EXPECT_EQ(config::drm_connector_from_sysfs_entry("card1-Virtual-1"), "Virtual-1");
  EXPECT_EQ(config::drm_connector_from_sysfs_entry("card2-HDMI-A-1"), "HDMI-A-1");
  EXPECT_EQ(config::drm_connector_from_sysfs_entry("card2-DP-10"), "DP-10");
  EXPECT_EQ(config::drm_connector_from_sysfs_entry("renderD128"), "");
}

TEST(ConfigConnectors, VirtualConnectorLists) {
  EXPECT_TRUE(config::is_config_virtual_connector("Virtual-1"));
  EXPECT_TRUE(config::is_config_left_virtual_connector("HERMES-1"));
  EXPECT_FALSE(config::is_config_virtual_connector("DP-1"));
}
