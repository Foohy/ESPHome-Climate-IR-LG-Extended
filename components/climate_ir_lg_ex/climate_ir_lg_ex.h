#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include <cinttypes>

namespace esphome {
namespace climate_ir_lg_ex {

// Temperature
const uint8_t TEMP_MIN_C = 16;
const uint8_t TEMP_MAX_C = 30;

const uint8_t TEMP_MIN_F = 60;
const uint8_t TEMP_MAX_F = 86;

class LgIrClimateEx : public climate_ir::ClimateIR {
 public:
  LgIrClimateEx()
    : climate_ir::ClimateIR(TEMP_MIN_C, TEMP_MAX_C, 1.0f, false, true,
    {climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_HIGH},
    {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL}) {}

    /// Override control to change settings of the climate device.
    void control(const climate::ClimateCall &call) override {
        send_swing_cmd_ = call.get_swing_mode().has_value();
        // swing resets after unit powered off
        if (call.get_mode().has_value() && *call.get_mode() == climate::CLIMATE_MODE_OFF)
            this->swing_mode = climate::CLIMATE_SWING_OFF;
        climate_ir::ClimateIR::control(call);
    }
    void set_header_high(uint32_t header_high) { this->header_high_ = header_high; }
    void set_header_low(uint32_t header_low) { this->header_low_ = header_low; }
    void set_bit_high(uint32_t bit_high) { this->bit_high_ = bit_high; }
    void set_bit_one_low(uint32_t bit_one_low) { this->bit_one_low_ = bit_one_low; }
    void set_bit_zero_low(uint32_t bit_zero_low) { this->bit_zero_low_ = bit_zero_low; }
    void set_flip_bit(int32_t bit_to_flip) { this->bit_to_flip_ = bit_to_flip; }

 protected:
    /// Transmit via IR the state of this climate controller.
    void transmit_state() override;
    /// Handle received IR Buffer
    bool on_receive(remote_base::RemoteReceiveData data) override;

    bool send_swing_cmd_{false};

    void calc_checksum_(uint8_t* data);
    void transmit_(const uint8_t* data);

    uint32_t header_high_;
    uint32_t header_low_;
    uint32_t bit_high_;
    uint32_t bit_one_low_;
    uint32_t bit_zero_low_;
    int32_t bit_to_flip_;

};

}  // namespace climate_ir_lg_ex
}  // namespace esphome
