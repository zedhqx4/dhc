#pragma once

#include <dinput.h>

#include <optional>
#include <variant>
#include <vector>

#include "dhc/dhc.h"
#include "dhc/logging.h"

#include "utils.h"

namespace dhc {

std::optional<uintptr_t> parse_dhc_guid(REFGUID guid);
GUID create_dhc_guid(uintptr_t index);

template <typename T>
struct DI8Types;

template <>
struct DI8Types<char> {
  using CharType = char;
  using InterfaceType = IDirectInput8A;
  using DeviceInterfaceType = IDirectInputDevice8A;
  using DeviceInstanceType = DIDEVICEINSTANCEA;
  using DeviceObjectInstanceType = DIDEVICEOBJECTINSTANCEA;
  using ActionFormatType = DIACTIONFORMATA;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSA;
  using EffectInfoType = DIEFFECTINFOA;
  using DeviceImageInfoHeaderType = DIDEVICEIMAGEINFOHEADERA;
};

template <>
struct DI8Types<wchar_t> {
  using CharType = wchar_t;
  using InterfaceType = IDirectInput8W;
  using DeviceInterfaceType = IDirectInputDevice8W;
  using DeviceInstanceType = DIDEVICEINSTANCEW;
  using DeviceObjectInstanceType = DIDEVICEOBJECTINSTANCEW;
  using ActionFormatType = DIACTIONFORMATW;
  using ConfigureDevicesParamsType = DICONFIGUREDEVICESPARAMSW;
  using EffectInfoType = DIEFFECTINFOW;
  using DeviceImageInfoHeaderType = DIDEVICEIMAGEINFOHEADERW;
};

template <typename CharType>
using DI8Interface = typename DI8Types<CharType>::InterfaceType;

template <typename CharType>
using DI8DeviceInterface = typename DI8Types<CharType>::DeviceInterfaceType;

template <typename CharType>
using DI8DeviceInstance = typename DI8Types<CharType>::DeviceInstanceType;

template <typename CharType>
using DI8DeviceObjectInstance = typename DI8Types<CharType>::DeviceObjectInstanceType;

template <typename CharType>
using DI8ActionFormat = typename DI8Types<CharType>::ActionFormatType;

template <typename CharType>
using DI8ConfigureDevicesParams = typename DI8Types<CharType>::ConfigureDevicesParamsType;

template <typename CharType>
using DI8EffectInfo = typename DI8Types<CharType>::EffectInfoType;

template <typename CharType>
using DI8DeviceImageInfoHeader = typename DI8Types<CharType>::DeviceImageInfoHeaderType;

// A struct representing an input for a given device.
struct EmulatedDeviceObject {
  const char* name;

  // GUID for the object type.
  GUID guid;

  // DIDFT_ABSAXIS, RELAXIS, PSHBUTTON, TGLBUTTON, POV, etc.
  // Note that several of the constants are bitmasks; the values here should be individual types.
  DWORD type;

  // DIDEVICEOBJECTINSTANCE::dwFlags
  // Should probably always contain DIDOI_POLLED?
  DWORD flags;

  // DIDFT_MAKEINSTANCE(instance_id)
  size_t instance_id;

  // Native data format of the object.
  // TODO: Does this matter?
  size_t offset;

  // Backend object that this object maps to, or std::monostate if it's unmapped.
  std::variant<std::monostate, AxisType, ButtonType, HatType> mapped_object;

  // Properties set by SetProperty:
  long range_min = 0;
  long range_max = 65535;

  double deadzone = 0.0;
  double saturation = 1.0;

  // Consumed by a DIOBJECTDATAFORMAT yet?
  bool matched = false;

  bool MatchesType(DWORD didft) {
    if (didft == DIDFT_ALL) {
      return true;
    }

    DWORD type_mask = DIDFT_GETTYPE(didft);
    if ((type_mask & type) == 0) {
      LOG(VERBOSE) << name << " type mismatch";
      return false;
    }
    didft &= ~type_mask;

    if ((didft & DIDFT_INSTANCEMASK) != DIDFT_ANYINSTANCE &&
        DIDFT_GETINSTANCE(didft) != instance_id) {
      LOG(VERBOSE) << name << " instance mismatch (want = " << DIDFT_GETINSTANCE(didft)
                   << ", self = " << instance_id << ")";
      return false;
    }
    didft &= ~DIDFT_INSTANCEMASK;

    didft &= ~DIDFT_OPTIONAL;

    if (didft != 0) {
      LOG(DEBUG) << "leftover flags: " << didft_to_string(didft);
      return false;
    }

    return true;
  }

  bool MatchesFlags(DWORD) const {
    // These seem to be ignored completely by Wine?
    return true;
  }

  DWORD Identifier() const { return type | DIDFT_MAKEINSTANCE(instance_id); }
};

std::vector<EmulatedDeviceObject> GeneratePS4EmulatedDeviceObjects();

struct DeviceFormat {
  observer_ptr<EmulatedDeviceObject> object;
  size_t offset;

  void Apply(char* output_buffer, size_t output_buffer_length,
             DeviceInputs inputs) const;
};

// Some fields (e.g. POV hats) need to be set to non-zero values if not found.
struct DeviceFormatDefault {
  size_t offset;
  DWORD value;
};

}  // namespace dhc
