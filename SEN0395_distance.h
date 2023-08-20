#include "esphome.h"
#include <string>

class Sen0395Distance : public PollingComponent, public UARTDevice
{
public:
  Sen0395Distance(UARTComponent *parent) : UARTDevice(parent), PollingComponent(1000) {}

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



  void setup() override
  {
    write_str("sensorStart");
    write_str("setUartOutput 2 1 0 1600");
  }

  void update() override
  {
    float t_snr[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    float t_distance[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool t_active[10] = {false, false, false, false, false, false, false, false, false, false};

    const int max_line_length = 40;
    static char buffer[max_line_length];

    if (!available())  // Request information if there is none on the UART buffer
    {
      this->write_str("getOutput");
      return;
    }
    while (available())
    {
      if (readline(read(), buffer, max_line_length) >= 4)
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

    // All the data is set
    setTargetValue(1, t_active[1], t_distance[1], t_snr[1]);
    setTargetValue(2, t_active[2], t_distance[2], t_snr[2]);
    setTargetValue(3, t_active[3], t_distance[3], t_snr[3]);
    setTargetValue(4, t_active[4], t_distance[4], t_snr[4]);
  }
};