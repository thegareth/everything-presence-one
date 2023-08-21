#include "esphome.h"
#include <string>
#include <tuple>

class Sen0395Distance : public PollingComponent, public UARTDevice
{
public:
  Sen0395Distance(UARTComponent *parent) : UARTDevice(parent), PollingComponent(1500) {}

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
  }

  void update() override
  {
    ESP_LOGD("custom", "Update loop called");

    typedef std::tuple<int, float, float> pointentry;

    std::vector<pointentry> sensorlines;

    std::string line;

    int current_result_count;
    const int max_line_length = 40;
    static char buffer[max_line_length];
    int line_count = 0;
    while (available())
    {
      if (available() && readline(read(), buffer, max_line_length) > 0)
      {
        line_count++;
        line = buffer;
        if (line.substr(0, 6) == "$JYRPO" && line.back() == '*')
        { // we have a sensor reading
          // store the number of results

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
          int total_targets = parse_number<int>(v[0]).value();
          int target_index = parse_number<int>(v[1]).value();
          float target_distance = parse_number<float>(v[2]).value();
          float target_snr = parse_number<float>(v[4]).value();

          if (target_index == 1)
          {
            ESP_LOGD("custom", "we hit a sensor clear point");
            sensorlines.clear();
          }
          sensorlines.push_back(std::make_tuple(target_index, target_distance, target_snr));
        }
      }
    }

    ESP_LOGD("custom", "Received %d lines, out of %d", sensorlines.size(), line_count);
    // if (sensorlines.size() > 0)
    // {
    //   ESP_LOGD("custom", "First line: %d, %f, %f - Last: %d, %f, %f", std::get<0>(sensorlines[0]), std::get<1>(sensorlines[0]), std::get<2>(sensorlines[0]),
    //            std::get<0>(sensorlines.back()), std::get<1>(sensorlines.back()), std::get<2>(sensorlines.back()));
    // }

    bool r_range_occupied[5] = {false, false, false, false, false};

    for (int range = 1; range < 4; range++)
    {
      if (this->f_range_start[range] >= 0 && this->f_range_end[range] > 0)
      {
        for (int target = 0; target < sensorlines.size(); target++)
        {
          if (this->f_range_start[range] <= std::get<1>(sensorlines[target]) && std::get<1>(sensorlines[target]) <= this->f_range_end[range])
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
  }
};