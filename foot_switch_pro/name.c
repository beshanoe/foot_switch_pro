#include "usb_names.h"

#define MIDI_NAME   {'f','o','o','t','_','s','w','i','t','c','h','_','p','r','o'}
#define MIDI_NAME_LEN  15

#define MANUFACTURER_NAME  {'B','e','s','h','a','n','o','e'}
#define MANUFACTURER_NAME_LEN 8

struct usb_string_descriptor_struct usb_string_product_name = {
  2 + MIDI_NAME_LEN * 2,
  3,
  MIDI_NAME
};

struct usb_string_descriptor_struct usb_string_manufacturer_name = {
  2 + MANUFACTURER_NAME_LEN * 2,
  3,
  MANUFACTURER_NAME
};
