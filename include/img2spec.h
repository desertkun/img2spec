
#include "device.h"

extern Device* img2spec_allocate_device(int deviceId);
extern void img2spec_free_device(Device* device);
extern uint8_t img2spec_load_workspace(const char *FileName, Device *gDevice);
extern uint8_t img2spec_load_image(const char *FileName, Device *gDevice);
extern void img2spec_process_image(Device *gDevice);

extern void img2spec_zx_spectrum_set_screen_order(Device *zx, int order);
extern void img2spec_zx_spectrum_set_screen_size(Device *zx, int w, int h);
extern void img2spec_set_scale(Device *device, float scale);
extern void img2spec_generate_result(Device *device, device_save_function_t cb);