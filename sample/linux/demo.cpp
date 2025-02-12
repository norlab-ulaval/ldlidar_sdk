/**
 * @file main.cpp
 * @author LDRobot (support@ldrobot.com)
 * @brief  main process App
 *         This code is only applicable to LDROBOT LiDAR LD00 LD03 LD08 LD14
 * products sold by Shenzhen LDROBOT Co., LTD
 * @version 0.1
 * @date 2021-11-08
 *
 * @copyright Copyright (c) 2017-2023  SHENZHEN LDROBOT CO., LTD. All rights
 * reserved.
 * Licensed under the MIT License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License in the file LICENSE
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ldlidar_driver/ldlidar_driver_linux.h"

uint64_t GetTimestamp(void) {
  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> tp = 
    std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now());
  auto tmp = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch());
  return ((uint64_t)tmp.count());
}

// void LidarPowerOn(void) {
//   LOG_DEBUG("Lidar Power On","");
//   // ...
// }

// void LidarPowerOff(void) {
//   LOG_DEBUG("Lidar Power Off","");
//   // ...
// }

struct LdsInfoStruct {
  std::string ldtype_str;
  ldlidar::LDType ldtype_enum;
  uint32_t baudrate;
};

LdsInfoStruct LdsInfoArrary[5] = {
  {"LD14", ldlidar::LDType::LD_14, 115200},
  {"LD14P", ldlidar::LDType::LD_14P, 230400},
  {"LD06", ldlidar::LDType::LD_06, 230400},
  {"LD19", ldlidar::LDType::LD_19, 230400},
  {"STL19P", ldlidar::LDType::STL_19P, 230400}
};

ldlidar::LDType GetLdsType(std::string in_str) {
  for (auto item : LdsInfoArrary) {
    if (!strcmp(in_str.c_str(), item.ldtype_str.c_str())) {
      return item.ldtype_enum;
    }
  }
  return ldlidar::LDType::NO_VER;
}

uint32_t GetLdsSerialPortBaudrateValue(std::string in_str) {
  for (auto item : LdsInfoArrary) {
    if (!strcmp(in_str.c_str(), item.ldtype_str.c_str())) {
      return item.baudrate;
    }
  }
  return 0;
}

int main(int argc, char **argv) {
  
  if (argc != 3) {
    LOG_INFO("cmd error","");
    LOG_INFO("please input: ./ldlidar <lidar type> <serial number>","");
    LOG_INFO("example:","");
    LOG_INFO("./ldlidar LD14 /dev/ttyUSB0","");
    LOG_INFO("./ldlidar LD14P /dev/ttyUSB0","");
    LOG_INFO("./ldlidar LD06 /dev/ttyUSB0","");
    LOG_INFO("./ldlidar LD19 /dev/ttyUSB0","");
    LOG_INFO("./ldlidar STL19P /dev/ttyUSB0","");
    exit(EXIT_FAILURE);
  }
  
  std::string ldlidar_type_str(argv[1]);
  std::string serial_port_name(argv[2]);

  // select ldrobot lidar sensor type.
  ldlidar::LDType ldlidar_type_dest;
  ldlidar_type_dest = GetLdsType(ldlidar_type_str);
  if (ldlidar_type_dest == ldlidar::LDType::NO_VER) {
    LOG_WARN("ldlidar_type_str value is not sure: %s", ldlidar_type_str.c_str());
    exit(EXIT_FAILURE);
  }

  // if use serial communications interface, as select serial baudrate paramters.
  uint32_t serial_baudrate_val;
  serial_baudrate_val = GetLdsSerialPortBaudrateValue(ldlidar_type_str);
  if (!serial_baudrate_val) {
    LOG_WARN("ldlidar_type_str value is not sure: %s", ldlidar_type_str.c_str());
    exit(EXIT_FAILURE);
  }
  
  ldlidar::LDLidarDriverLinuxInterface* lidar_drv = 
    ldlidar::LDLidarDriverLinuxInterface::Create();
  
  LOG_INFO("LDLiDAR SDK Pack Version is %s", lidar_drv->GetLidarSdkVersionNumber().c_str());

  lidar_drv->RegisterGetTimestampFunctional(std::bind(&GetTimestamp)); 

  lidar_drv->EnablePointCloudDataFilter(true);

  if (lidar_drv->Connect(ldlidar_type_dest, serial_port_name, serial_baudrate_val)) {
    LOG_INFO("ldlidar serial connect is success","");
    // LidarPowerOn();
  } else {
    LOG_ERROR("ldlidar serial connect is fail","");
    exit(EXIT_FAILURE);
  }

  if (lidar_drv->WaitLidarComm(3500)) {
    LOG_INFO("ldlidar communication is normal.","");
  } else {
    LOG_ERROR("ldlidar communication is abnormal.","");
    lidar_drv->Disconnect();
  }

  if (lidar_drv->Start()) {
    LOG_INFO_LITE("ldlidar driver start is success.","");
  } else {
    LOG_ERROR_LITE("ldlidar driver start is fail.","");
  }
  
  ldlidar::Points2D laser_scan_points;
  while (ldlidar::LDLidarDriverLinuxInterface::Ok()) {

    switch (lidar_drv->GetLaserScanData(laser_scan_points, 1500)){
      case ldlidar::LidarStatus::NORMAL: {
        double lidar_scan_freq = 0;
        lidar_drv->GetLidarScanFreq(lidar_scan_freq);
#ifdef __LP64__
        LOG_INFO_LITE("speed(Hz):%f, size:%d,stamp_begin:%lu, stamp_end:%lu",
            lidar_scan_freq, laser_scan_points.size(), laser_scan_points.front().stamp, laser_scan_points.back().stamp);
#else
        LOG_INFO_LITE("speed(Hz):%f, size:%d,stamp_begin:%llu, stamp_end:%llu",
            lidar_scan_freq, laser_scan_points.size(), laser_scan_points.front().stamp, laser_scan_points.back().stamp);
#endif
        //  output 2d point cloud data
#if 0
        for (auto point : laser_scan_points) {
#ifdef __LP64__
          LOG_INFO_LITE("stamp(ns):%lu,angle:%f,distance(mm):%d,intensity:%d", 
              point.stamp, point.angle, point.distance, point.intensity);
#else
          LOG_INFO_LITE("stamp(ns):%llu,angle:%f,distance(mm):%d,intensity:%d", 
              point.stamp, point.angle, point.distance, point.intensity);
#endif
        }
#endif
        break;
      }
      case ldlidar::LidarStatus::DATA_TIME_OUT: {
        LOG_ERROR_LITE("point cloud data publish time out, please check your lidar device.","");
        lidar_drv->Stop();
        break;
      }
      case ldlidar::LidarStatus::DATA_WAIT: {
        break;
      }
      default:
        break;
    }

    usleep(1000*166);  // sleep 166ms , 6hz
  }

  lidar_drv->Stop();
  lidar_drv->Disconnect();
  // LidarPowerOff();

  ldlidar::LDLidarDriverLinuxInterface::Destory(lidar_drv);

  return 0;
}

/********************* (C) COPYRIGHT SHENZHEN LDROBOT CO., LTD *******END OF
 * FILE ********/
