#include "esphome.h"
#include <string>

class Sen0395Distance : public PollingComponent, public UARTDevice
{
public:
  Sen0395Distance(UARTComponent *parent) : UARTDevice(parent), PollingComponent(1000) {}

  bool b_write_target_details = false;
  float f_range_start[5] = {0, 0, 0, 0, 0};
  float f_range_end[5] = {0, 0, 0, 0, 0};

  void update_config()
  {
    ESP_LOGD("custom", "Config update called");
    this->b_write_target_details = id(target_output).state / 100.0;
    this->f_range_start[1] = id(mmwave_zone_1_start).state / 100.0;
    this->f_range_end[1] = id(mmwave_zone_1_end).state / 100.0;
    this->f_range_start[2] = id(mmwave_zone_2_start).state / 100.0;
    this->f_range_end[2] = id(mmwave_zone_2_end).state / 100.0;
    this->f_range_start[3] = id(mmwave_zone_3_start).state / 100.0;
    this->f_range_end[3] = id(mmwave_zone_3_end).state / 100.0;
    this->f_range_start[4] = id(mmwave_zone_4_start).state / 100.0;
    this->f_range_end[4] = id(mmwave_zone_4_end).state / 100.0;
    ESP_LOGD("custom", "MMWave1: %f, %f", this->f_range_start[1], this->f_range_end[1]);
  }

  int readline(int readch, char *buffer, int len)
  {
    static int pos = 0;
    int rpos;

    if (readch > 0)
    {
      switch (readch)
      {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0; // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len - 1)
        {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
      }
    }
    // No end of line has been found, so return -1.
    return -1;
  }

  void setTargetValue(int sensorIndex, bool isActive, float distance, float snr)
  {
    if (!this->b_write_target_details)
    {
      return;
    }
    // If inactive, remove values for distance & SNR
    float resolved_distance = isActive ? distance : 0.0;
    float resolved_snr = isActive ? snr : 0.0;

    if (sensorIndex == 1)
    {
      id(target_1_active).publish_state(isActive);
      id(target_1_distance).publish_state(resolved_distance);
      id(target_1_snr).publish_state(resolved_snr);
    }

    if (sensorIndex == 2)
    {
      id(target_2_active).publish_state(isActive);
      id(target_2_distance).publish_state(resolved_distance);
      id(target_2_snr).publish_state(resolved_snr);
    }

    if (sensorIndex == 3)
    {
      id(target_3_active).publish_state(isActive);
      id(target_3_distance).publish_state(resolved_distance);
      id(target_3_snr).publish_state(resolved_snr);
    }

    if (sensorIndex == 4)
    {
      id(target_4_active).publish_state(isActive);
      id(target_4_distance).publish_state(resolved_distance);
      id(target_4_snr).publish_state(resolved_snr);
    }
  }

  void markTargetIdle(int sensorIndex)
  {
    setTargetValue(sensorIndex, false, 0, 0);
  }

  void setup() override
  {
    write_str("setUartOutput 1 0");
    delay(1000);
    write_str("setUartOutput 2 1 0 1600");
    delay(1000);
    write_str("sensorStart");
  }

  void update() override
  {
    ESP_LOGD("custom", "Update loop called");
    float t_snr[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    float t_distance[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool t_active[10] = {false, false, false, false, false, false, false, false, false, false};
    bool r_range_occupied[5] = {false, false, false, false, false};

    const int max_line_length = 40;
    static char buffer[max_line_length];

    while (available())
    {
      if (readline(read(), buffer, max_line_length) >= 3)
      {
        std::string line = buffer;
        if (line.substr(0, 6) == "$JYRPO")
        {
          std::string vline = line.substr(6);
          std::vector<std::string> v;
          for (int i = 0; i < vline.length(); i++)
          {
            if (vline[i] == ',')
            {
              v.push_back("");
            }
            else
            {
              v.back() += vline[i];
            }
          }
          int target_count = parse_number<int>(v[0]).value();
          int target_index = parse_number<int>(v[1]).value();
          float target_distance = parse_number<float>(v[2]).value();
          float target_snr = parse_number<float>(v[4]).value();

          ESP_LOGD("custom", "Target %d/%d Distance: %f, SNR: %f", target_index, target_count, target_distance, target_snr);
          t_snr[target_index] = target_snr;
          t_distance[target_index] = target_distance;
          t_active[target_index] = true;
        }
      }
    }

    for (int range = 1; range < 4; range++)
    {
      if (this->f_range_start[range] >= 0 && this->f_range_end[range] > 0)
      {
        for (int target = 1; target < 10; target++)
        {
          if (t_active[target] && t_distance[target] >= this->f_range_start[range] && t_distance[target] <= this->f_range_end[range])
          {
            r_range_occupied[range] = true;
          }
        }
      }
    }

    id(r1_occupied).publish_state(r_range_occupied[1]);
    id(r2_occupied).publish_state(r_range_occupied[2]);
    id(r3_occupied).publish_state(r_range_occupied[3]);
    id(r4_occupied).publish_state(r_range_occupied[4]);

    // All the data is set
    setTargetValue(1, t_active[1], t_distance[1], t_snr[1]);
    setTargetValue(2, t_active[2], t_distance[2], t_snr[2]);
    setTargetValue(3, t_active[3], t_distance[3], t_snr[3]);
    setTargetValue(4, t_active[4], t_distance[4], t_snr[4]);

    this->write_str("getOutput");

  }
};