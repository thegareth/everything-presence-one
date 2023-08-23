#include "esphome.h"
#include <string>
#include <tuple>

class Sen0395Distance : public UARTDevice, public PollingComponent
{
public:
  Sen0395Distance(UARTComponent *parent) : UARTDevice(parent), PollingComponent(1500) {}

  bool b_write_target_details = false;
  float f_range_start[5] = {0, 0, 0, 0, 0};
  float f_range_end[5] = {0, 0, 0, 0, 0};
  typedef std::tuple<int, float, float> pointentry;


  void update_config()
  {
    ESP_LOGD("custom", "Config update called");
    this->b_write_target_details = id(target_output).state;
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
    // ESP_LOGD("custom", "Update loop called");

    std::vector<pointentry> sensorlines;

    std::string line;

    int current_result_count;
    const int max_line_length = 40;
    static char buffer[max_line_length];
    int line_count = 0;
    while (available())

    // If there is UART sensor data from the MMWave, read it in.

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
            // When the target index is 1, that means we're starting a new set of readings
            // Re-setting the vector clears out any old readings and returns only one set
            sensorlines.clear();
          }
          // Pushing the sensor readings into the vector as a tuple, so we can access them later
          sensorlines.push_back(std::make_tuple(target_index, target_distance, target_snr));
        }
      }
    }

    // DEBUG that shows how many lines we are using versus how many we received
    // ESP_LOGD("custom", "Selected %d lines, out of %d received", sensorlines.size(), line_count);
    // if (sensorlines.size() > 0)

    // Now we have the latest set of readings from the sensor, we can iterate over them and figure out which zones are occupied
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

    // We can then publish those
    // TODO refactor this to a function that we can loop

    id(r1_occupied).publish_state(r_range_occupied[1]);
    id(r2_occupied).publish_state(r_range_occupied[2]);
    id(r3_occupied).publish_state(r_range_occupied[3]);
    id(r4_occupied).publish_state(r_range_occupied[4]);

    // We can also do similar for the target: their active state, distance and SNR
    float t_snr[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    float t_distance[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool t_active[10] = {false, false, false, false, false, false, false, false, false, false};

    for (int target = 0; target < sensorlines.size(); target++)
    {
      pointentry current_target = sensorlines[target];
      int target_index = std::get<0>(current_target);
      t_active[target_index] = true;
      t_distance[target_index] = std::get<1>(current_target);
      t_snr[target_index] = std::get<2>(current_target);
    }

    // Call update target for each target up to 4
    for (int target = 1; target < 5; target++)
    {
        setTargetValue(target, t_active[target], t_distance[target], t_snr[target]);
    }

  }
};