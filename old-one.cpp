#include "esphome.h"
#include <string>

class Sen0395Distance : public Component, public UARTDevice {
public:
  Sen0395Distance(UARTComponent *parent) : UARTDevice(parent) {}

  float t_snr[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  float t_distance[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  bool t_active[10] = {false, false, false, false, false, false, false, false, false, false};

  int max_targets = 4;

  void setup() override {
    // nothing to do here
  }

  int readline(int readch, char *buffer, int len) {
    static int pos = 0;
    int rpos;

    if (readch > 0) {
      switch (readch) {
        case '\n': // Ignore new-lines
          break;
        case '\r': // Return on CR
          rpos = pos;
          pos = 0; // Reset position index ready for next time
          return rpos;
        default:
          if (pos < len - 1) {
            buffer[pos++] = readch;
            buffer[pos] = 0;
          }
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }

  void setTargetValue(int sensorIndex, bool isActive, float distance, float snr){
    // If inactive, remove values for distance & SNR
    float resolved_distance = isActive ? distance : 0.00000001;
    float resolved_snr = isActive ? snr : 0.00000001;

    if(sensorIndex==1){
      id(target_1_active).publish_state(isActive);
      id(target_1_distance).publish_state(resolved_distance);
      id(target_1_snr).publish_state(resolved_snr);
    }

    if(sensorIndex==2){
      id(target_2_active).publish_state(isActive);
      id(target_2_distance).publish_state(resolved_distance);
      id(target_2_snr).publish_state(resolved_snr);
    }

    if(sensorIndex==3){
      id(target_3_active).publish_state(isActive);
      id(target_3_distance).publish_state(resolved_distance);
      id(target_3_snr).publish_state(resolved_snr);
    }

    if(sensorIndex==4){
      id(target_4_active).publish_state(isActive);
      id(target_4_distance).publish_state(resolved_distance);
      id(target_4_snr).publish_state(resolved_snr);
    }

  }

  void markTargetIdle(int sensorIndex){
    setTargetValue(sensorIndex, false, 0, 0);
  }

  void loop() override {
    const int max_line_length = 40;
    static char buffer[max_line_length];

    while (available()) {
      if (readline(read(), buffer, max_line_length) >= 4) {
        std::string line = buffer;

        if (line.substr(0, 6) == "$JYRPO") {
          std::string vline = line.substr(6);
          std::vector<std::string> v;
          for (int i = 0; i < vline.length(); i++) {
            if (vline[i] == ',') {
              v.push_back("");
            } else {
              v.back() += vline[i];
            }
          }
          int target_count = parse_number<int>(v[0]).value();
          int target_index = parse_number<int>(v[1]).value();
          float target_distance = parse_number<float>(v[2]).value();
          float target_snr = parse_number<float>(v[4]).value();

          // Update the SNR, distance, and active status for the current target index
          t_snr[target_index] = target_snr;
          t_distance[target_index] = target_distance;
          t_active[target_index] = true;

          int last_active = 0;

          for (int i = 1; i <= target_count; i++) {
            setTargetValue(i, t_active[i], t_distance[i], t_snr[i]);
            if (t_active){
              last_active = i;
            }
          }

          for (int i = last_active; i <= max_targets; i++) {
            // ESP_LOGD("custom", "marking as idle: %d", i);
            markTargetIdle(i);
          }

          // if (target_count == target_index) {
          //   // Publish the data for each target without sorting
          //   for (int i = 1; i <= target_count; i++) {
          //     auto get_sensors = App.get_sensors();
          //     for (int j = 0; j < get_sensors.size(); j++) {
          //       std::string name = get_sensors[j]->get_name();
          //       if (name == "Target " + std::to_string(i) + " Distance") {
          //         get_sensors[j]->publish_state(t_distance[i]);
          //       } else if (name == "Target " + std::to_string(i) + " SNR") {
          //         get_sensors[j]->publish_state(t_snr[i]);
          //       }
          //     }
          //   }

          //   // Publish the state of the binary sensor for each target
          //   for (int i = 1; i <= 10; i++) {
          //     auto get_binary_sensors = App.get_binary_sensors();
          //     for (int j = 0; j < get_binary_sensors.size(); j++) {
          //       std::string name = get_binary_sensors[j]->get_name();
          //       if (name == "Target " + std::to_string(i) + " Active") {
          //         get_binary_sensors[j]->publish_state(t_active[i]);
          //       }
          //     }
          //   }
            
          //   for (int i = target_count + 1; i <= 4; i++) {
          //     markTargetIdle(i);
          //   }


            //ESP_LOGD("custom", "Target %d Distance: %f, SNR: %f", i, t_distance[i], t_snr[i]);
        }
      }
    }
  }
};
