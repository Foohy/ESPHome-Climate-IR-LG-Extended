#include "climate_ir_lg_ex.h"
#include "esphome/core/log.h"

#include <sstream>
#include <iomanip>

namespace esphome {
namespace climate_ir_lg_ex {

static const char *const TAG = "climate.climate_ir_lg_ex";

// State description
union LGProtocolState
{
  uint8_t raw[14];

  struct
  {
    // Bytes 1-4 are some sort of address
    uint8_t AddressA      :8;
    uint8_t AddressB      :8;
    uint8_t AddressC      :8;
    uint8_t AddressD      :8;

    // Byte 5 is all zeroes
    uint8_t UnknownA       :8;

    // Byte 6
    uint8_t               :2;
    uint8_t Power         :1; // On or off
    uint8_t TimerEnable   :1; // Sets the timer counting down
    uint8_t UnknownB      :2;
    uint8_t LightToggle   :1; // If set, toggles the LED backlight intensity (no way to set)
    uint8_t               :1;

    // Byte 7
    uint8_t Mode          :3; // Operating mode
    uint8_t               :5;


    // Byte 8
    uint8_t TempCelsius   :4; // Temperature to set if in celsius mode
    uint8_t               :4;

    // Byte 9
    uint8_t FanSpeed      :3; // Set the fan speed
    uint8_t Swing         :3; // Set the fan swing mode
    uint8_t TimerReset    :1; // If this is set, restarts the timer to whatever we just sent
    uint8_t               :1;

    // Byte 10
    uint8_t               :1;
    uint8_t TimerHour     :7; // Set how long to wait for timer

    // Byte 11
    uint8_t               :8;

    // Byte 12
    uint8_t               :8;

    // Byte 13
    uint8_t TempFarenheit :7; // Temperature to set if in fart mode
    uint8_t TempUnit      :1; // Determines whether to use Celsius (0) or Fart (1)


    // Byte 14 is checksum
    uint32_t Checksum     :8;
  };
};

// Message data
const uint8_t FRAME_SIZE        = 14;

static const uint32_t ADDRESS_A = 0x23;
static const uint32_t ADDRESS_B = 0xCB;
static const uint32_t ADDRESS_C = 0x26;
static const uint32_t ADDRESS_D = 0x01;


// Commands
const uint8_t COMMAND_MODE_DRY  = 0x2;
const uint8_t COMMAND_MODE_COOL = 0x3;
const uint8_t COMMAND_MODE_FAN  = 0x7;
const uint8_t COMMAND_MODE_OFF  = 0x0;

const uint8_t COMMAND_FAN_MIN   = 0x2;
const uint8_t COMMAND_FAN_MAX   = 0x5;

const uint8_t COMMAND_SWING_OFF   = 0x0;
const uint8_t COMMAND_SWING_VERT  = 0x7;


/*
const uint32_t COMMAND_MASK = 0xFF000;
const uint32_t COMMAND_OFF = 0xC0000;
const uint32_t COMMAND_SWING = 0x10000;

const uint32_t COMMAND_ON_COOL = 0x00000;
const uint32_t COMMAND_ON_DRY = 0x01000;
const uint32_t COMMAND_ON_FAN_ONLY = 0x02000;
const uint32_t COMMAND_ON_AI = 0x03000;
const uint32_t COMMAND_ON_HEAT = 0x04000;

const uint32_t COMMAND_COOL = 0x08000;
const uint32_t COMMAND_DRY = 0x09000;
const uint32_t COMMAND_FAN_ONLY = 0x0A000;
const uint32_t COMMAND_AI = 0x0B000;
const uint32_t COMMAND_HEAT = 0x0C000;

// Fan speed
const uint32_t FAN_MASK = 0xF0;
const uint32_t FAN_AUTO = 0x50;
const uint32_t FAN_MIN = 0x00;
const uint32_t FAN_MED = 0x20;
const uint32_t FAN_MAX = 0x40;

// Temperature
const uint8_t TEMP_RANGE = TEMP_MAX - TEMP_MIN + 1;
const uint32_t TEMP_MASK = 0XF00;
const uint32_t TEMP_SHIFT = 8;
*/

static float fart_to_celsius(float fart)
{
  return ((fart - 32.f) * 5.f) / 9.f;
}

static float celsius_to_fart(float celsius)
{
  return ((celsius * 9.f) / 5.f) + 32.f;
}

void LgIrClimateEx::transmit_state() {
    LGProtocolState remote_state;
    memset(remote_state.raw, 0, sizeof(remote_state));

    // Setup message parameters
    {
        remote_state.AddressA = ADDRESS_A;
        remote_state.AddressB = ADDRESS_B;
        remote_state.AddressC = ADDRESS_C;
        remote_state.AddressD = ADDRESS_D;
        remote_state.UnknownB = 0x2;
    }

    // Set operating mode
    {
        switch (this->mode) {
        case climate::CLIMATE_MODE_COOL:
            remote_state.Mode = COMMAND_MODE_COOL;
            break;
        case climate::CLIMATE_MODE_DRY:
            remote_state.Mode = COMMAND_MODE_DRY;
            break;
        case climate::CLIMATE_MODE_FAN_ONLY:
            remote_state.Mode = COMMAND_MODE_FAN;
            break;
        case climate::CLIMATE_MODE_OFF:
        default:
            remote_state.Mode = COMMAND_MODE_OFF;
            break;
        }

        // Toggle power state
        remote_state.Power = remote_state.Mode != COMMAND_MODE_OFF;
    }

    // Set fan speed
    {
        switch (this->fan_mode.value()) {
        case climate::CLIMATE_FAN_HIGH:
            remote_state.FanSpeed = COMMAND_FAN_MAX;
            break;
        case climate::CLIMATE_FAN_LOW:
        default:
            remote_state.FanSpeed = COMMAND_FAN_MIN;
            break;
        }
    }

    // Set target temperature
    {
        uint8_t clamped_celsius = (uint8_t)roundf(clamp<float>(this->target_temperature, TEMP_MIN_C, TEMP_MAX_C));
        uint8_t clamped_fart    = (uint8_t)roundf(clamp<float>(celsius_to_fart(this->target_temperature), TEMP_MIN_F, TEMP_MAX_F));
        remote_state.TempCelsius    = 0x1F - clamped_celsius; // Celsius encoded a bit strangely
        remote_state.TempFarenheit  = clamped_fart;
        remote_state.TempUnit       = 1; // we fart in this house

        ESP_LOGD(TAG, "Setting temp to %d F (%d C) (Input: %f)", remote_state.TempFarenheit, remote_state.TempCelsius, this->target_temperature);
    }

    // Set swing state
    {
      switch (this->swing_mode) {
      case climate::CLIMATE_SWING_OFF:
        remote_state.Swing = COMMAND_SWING_OFF;
        break;
      case climate::CLIMATE_SWING_VERTICAL:
      default:
        remote_state.Swing = COMMAND_SWING_VERT;
        break;
      }
    }

    // Flip a single bit maybe
    {
      int32_t in_byte = bit_to_flip_ / 8;
      uint8_t flip_mask = 1U << (bit_to_flip_ % 8);
      if (in_byte >= 0 && in_byte <= sizeof(remote_state))
      {
        remote_state.raw[in_byte] ^= flip_mask;
        ESP_LOGD(TAG, "Flipping bit %d (bit %d in byte %d)", bit_to_flip_, (bit_to_flip_ % 8), in_byte);
      }
    }

    calc_checksum_(remote_state.raw);
    transmit_(remote_state.raw);
    this->publish_state();
}

bool LgIrClimateEx::on_receive(remote_base::RemoteReceiveData data) {
    /*
  uint8_t nbits = 0;
  uint32_t remote_state = 0;

  if (!data.expect_item(this->header_high_, this->header_low_))
    return false;

  for (nbits = 0; nbits < 32; nbits++) {
    if (data.expect_item(this->bit_high_, this->bit_one_low_)) {
      remote_state = (remote_state << 1) | 1;
    } else if (data.expect_item(this->bit_high_, this->bit_zero_low_)) {
      remote_state = (remote_state << 1) | 0;
    } else if (nbits == BITS) {
      break;
    } else {
      return false;
    }
  }

  ESP_LOGD(TAG, "Decoded 0x%02" PRIX32, remote_state);
  if ((remote_state & 0xFF00000) != 0x8800000)
    return false;

  // Get command
  if ((remote_state & COMMAND_MASK) == COMMAND_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
  } else if ((remote_state & COMMAND_MASK) == COMMAND_SWING) {
    this->swing_mode =
        this->swing_mode == climate::CLIMATE_SWING_OFF ? climate::CLIMATE_SWING_VERTICAL : climate::CLIMATE_SWING_OFF;
  } else {
    switch (remote_state & COMMAND_MASK) {
      case COMMAND_DRY:
      case COMMAND_ON_DRY:
        this->mode = climate::CLIMATE_MODE_DRY;
        break;
      case COMMAND_FAN_ONLY:
      case COMMAND_ON_FAN_ONLY:
        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
        break;
      case COMMAND_AI:
      case COMMAND_ON_AI:
        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
        break;
      case COMMAND_HEAT:
      case COMMAND_ON_HEAT:
        this->mode = climate::CLIMATE_MODE_HEAT;
        break;
      case COMMAND_COOL:
      case COMMAND_ON_COOL:
      default:
        this->mode = climate::CLIMATE_MODE_COOL;
        break;
    }

    // Get fan speed
    if (this->mode == climate::CLIMATE_MODE_HEAT_COOL) {
      this->fan_mode = climate::CLIMATE_FAN_AUTO;
    } else if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_DRY ||
               this->mode == climate::CLIMATE_MODE_FAN_ONLY || this->mode == climate::CLIMATE_MODE_HEAT) {
      if ((remote_state & FAN_MASK) == FAN_AUTO) {
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
      } else if ((remote_state & FAN_MASK) == FAN_MIN) {
        this->fan_mode = climate::CLIMATE_FAN_LOW;
      } else if ((remote_state & FAN_MASK) == FAN_MED) {
        this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
      } else if ((remote_state & FAN_MASK) == FAN_MAX) {
        this->fan_mode = climate::CLIMATE_FAN_HIGH;
      }
    }

    // Get temperature
    if (this->mode == climate::CLIMATE_MODE_COOL || this->mode == climate::CLIMATE_MODE_HEAT) {
      this->target_temperature = ((remote_state & TEMP_MASK) >> TEMP_SHIFT) + 15;
    }
  }
  this->publish_state();

  return true;
*/
    // TODO: Decode state
    return false;
}

void LgIrClimateEx::transmit_(const uint8_t* data) {
  
    char buffer[50];
    std::string output;
    for (int i = 0; i < FRAME_SIZE; ++i) {
        snprintf(buffer, sizeof(buffer), " %02x", data[i]);
        output += std::string(buffer);
    }
    ESP_LOGD(TAG, "Sending climate_ir_lg_ex state: %s", output.c_str());

    auto transmit = this->transmitter_->transmit();
    auto *transmit_data = transmit.get_data();

    transmit_data->set_carrier_frequency(38000);

    transmit_data->item(this->header_high_, this->header_low_);

    // Loop over each byte
    for (int i = 0; i < FRAME_SIZE; ++i) {
        uint8_t byte = data[i];

        // Loop over each bit
        for (uint8_t b = 0; b < 8; ++b) {
            uint8_t mask = 1U << b;
            if (byte & mask) {
                transmit_data->item(this->bit_high_, this->bit_one_low_);
            } else {
                transmit_data->item(this->bit_high_, this->bit_zero_low_);
            }
        }
    }

  transmit_data->mark(this->bit_high_);

  transmit.perform();
}
void LgIrClimateEx::calc_checksum_(uint8_t* data) {

    // Last byte contains the sum of all previous bytes
    uint8_t sum = 0;
    uint32_t i;
    for (i = 0; i < FRAME_SIZE - 1; i++)
    {
        sum += data[i];
    }
    data[i] = sum;
}

}  // namespace climate_ir_lg_ex
}  // namespace esphome
