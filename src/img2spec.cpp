
#include <cstdio>
#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>

#include "parson/parson.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define VERSION "4.0"

#define SERIALIZE(x) json_object_dotset_number(root, #x, x);
#define DESERIALIZE(x) if (json_object_dotget_value(root, #x) != NULL) x = json_object_dotget_number(root, #x);

#include "common.h"

enum MODIFIERS
{
    MOD_SCALEPOS = 1,
    MOD_RGB,
    MOD_YIQ,
    MOD_HSV,
    MOD_NOISE,
    MOD_ORDEREDDITHER,
    MOD_ERRORDIFFUSION,
    MOD_CONTRAST,
    MOD_BLUR,
    MOD_EDGE,
    MOD_MINMAX,
    MOD_QUANTIZE,
    MOD_SUPERBLACK,
    MOD_CURVE
};

struct ImVec2
{
    float x, y;
    ImVec2() { x = y = 0.0f; }
    ImVec2(float _x, float _y) { x = _x; y = _y; }
};

struct ImVec4
{
    float x, y, z, w;
    ImVec4() { x = y = z = w = 0.0f; }
    ImVec4(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
};


char *gSourceImageName = 0;
unsigned int *gSourceImageData = 0;
int gSourceImageX = 0, gSourceImageY = 0;

class Device;

void bitmap_to_float(unsigned int *aBitmap, Device *gDevice);
int float_to_color(float aR, float aG, float aB);

int gUniqueValueCounter = 0;

#define NO_GUI

#include "device.h"
#include "modifier.h"

// Floating point version of the processed bitmap, for processing
float gBitmapProcFloat[1024 * 512 * 3];

// Bitmaps for the textures
unsigned int gBitmapOrig[1024 * 512];
unsigned int gBitmapProc[1024 * 512];
unsigned int gBitmapSpec[1024 * 512];
unsigned int gBitmapAttr[1024 * 512];
unsigned int gBitmapAttr2[1024 * 512];
unsigned int gBitmapBitm[1024 * 512];

#include "zxspectrumdevice.h"
#include "zx3x64device.h"
#include "zxhalftiledevice.h"
#include "c64hiresdevice.h"
#include "c64multicolordevice.h"

#include "scaleposmodifier.h"
#include "rgbmodifier.h"
#include "yiqmodifier.h"
#include "hsvmodifier.h"
#include "noisemodifier.h"
#include "blurmodifier.h"
#include "edgemodifier.h"
#include "quantizemodifier.h"
#include "minmaxmodifier.h"
#include "ordereddithermodifier.h"
#include "errordiffusiondithermodifier.h"
#include "contrastmodifier.h"
#include "superblackmodifier.h"
#include "curvemodifier.h"

class Modifier;

char * mystrdup(char * aString)
{
    int len = 0;
    while (aString[len]) len++;
    char * data = new char[len + 1];
    memcpy(data, aString, len);
    data[len] = 0;
    return data;
}

uint8_t img2spec_load_image(const char *FileName, Device *gDevice)
{
    FILE *f = fopen(FileName, "rb");
    if (!f)
        return 1;
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fclose(f);

    // gSourceImageName may be the same pointer as aFilename, so freeing it first
    // and then trying to duplicate it to itself might... well..
    char *name = mystrdup((char *)FileName);
    int n, iters = 0, x, y;
    unsigned int *data = nullptr;
    while (data == nullptr && iters < 16)
    {
        data = (unsigned int*)stbi_load(name, &x, &y, &n, 4);
        iters++;
    }

    if (data == nullptr)
    {
        return 1;
    }

    gSourceImageX = x;
    gSourceImageY = y;

    if (gSourceImageName)
        delete[] gSourceImageName;

    gSourceImageName = name;
    if (gSourceImageData)
        stbi_image_free(gSourceImageData);
    gSourceImageData = data;


    int i, j;
    for (i = 0; i < gDevice->mYRes; i++)
    {
        for (j = 0; j < gDevice->mXRes; j++)
        {
            int pix = 0xff000000;
            if (j < gSourceImageX && i < gSourceImageY)
                pix = gSourceImageData[i * gSourceImageX + j] | 0xff000000;
            gBitmapOrig[i * gDevice->mXRes + j] = pix;
        }
    }
    return 0;
}

// Should probably add to the end of the list instead of beginning..
void addModifier(Modifier *aNewModifier, Device* gDevice)
{
    aNewModifier->mNext = gDevice->gModifierRoot;
    gDevice->gModifierRoot = aNewModifier;
}

uint8_t img2spec_load_workspace(const char *FileName, Device *gDevice)
{
    JSON_Value *root_value = json_parse_file(FileName);
    if (root_value)
    {
        JSON_Object *root = json_value_get_object(root_value);
        if (_stricmp(json_object_dotget_string(root, "About.Magic"), "0x50534D49") == 0 &&
            json_object_dotget_number(root, "About.Version") == 4)
        {
            gDevice->readOptions(root);

            Modifier *walker = gDevice->gModifierRoot;
            while (walker)
            {
                Modifier *last = walker;
                walker = walker->mNext;
                delete last;
            }
            gDevice->gModifierRoot = 0;

            int number = 0;
            char path[256];
            sprintf(path, "Stack.Item[%d]", number);
            JSON_Object *item = json_object_dotget_object(root, path);

            while (item)
            {

                int m;
                if (json_object_get_value(item, "Type"))
                {
                    m = (int)json_object_get_number(item, "Type");

                    Modifier *n = 0;
                    switch (m)
                    {
                        case MOD_SCALEPOS: n = new ScalePosModifier; break;
                        case MOD_RGB: n = new RGBModifier; break;
                        case MOD_YIQ: n = new YIQModifier; break;
                        case MOD_HSV: n = new HSVModifier; break;
                        case MOD_NOISE: n = new NoiseModifier; break;
                        case MOD_ORDEREDDITHER: n = new OrderedDitherModifier; break;
                        case MOD_ERRORDIFFUSION: n = new ErrorDiffusionDitherModifier; break;
                        case MOD_CONTRAST: n = new ContrastModifier; break;
                        case MOD_BLUR: n = new BlurModifier; break;
                        case MOD_EDGE: n = new EdgeModifier; break;
                        case MOD_MINMAX: n = new MinmaxModifier; break;
                        case MOD_QUANTIZE: n = new QuantizeModifier; break;
                        case MOD_SUPERBLACK: n = new SuperblackModifier; break;
                        case MOD_CURVE: n = new CurveModifier; break;
                        default:
                            json_value_free(root_value);
                            return 1;
                    }
                    addModifier(n, gDevice);
                    n->deserialize_common(item);
                    n->deserialize(item);
                }
                number++;
                sprintf(path, "Stack.Item[%d]", number);
                item = json_object_dotget_object(root, path);
            }
        }
        json_value_free(root_value);
    }
    else
    {
        return 1;
    }

    return 0;
}

void bitmap_to_float(unsigned int *aBitmap, Device *gDevice)
{
    int i;
    for (i = 0; i < gDevice->mXRes * gDevice->mYRes; i++)
    {
        int c = aBitmap[i];
        int r = (c >> 16) & 0xff;
        int g = (c >> 8) & 0xff;
        int b = (c >> 0) & 0xff;

        gBitmapProcFloat[i * 3 + 0] = r * (1.0f / 255.0f);
        gBitmapProcFloat[i * 3 + 1] = g * (1.0f / 255.0f);
        gBitmapProcFloat[i * 3 + 2] = b * (1.0f / 255.0f);
    }
}

int float_to_color(float aR, float aG, float aB)
{
    aR = (aR < 0) ? 0 : (aR > 1) ? 1 : aR;
    aG = (aG < 0) ? 0 : (aG > 1) ? 1 : aG;
    aB = (aB < 0) ? 0 : (aB > 1) ? 1 : aB;

    return ((int)floor(aR * 255) << 16) |
           ((int)floor(aG * 255) << 8) |
           ((int)floor(aB * 255) << 0);
}

void build_applystack(Device* gDevice)
{
    gDevice->gModifierApplyStack = gDevice->gModifierRoot;
    if (gDevice->gModifierApplyStack)
    {
        gDevice->gModifierApplyStack->mApplyNext = 0;

        Modifier *walker = gDevice->gModifierRoot->mNext;
        while (walker)
        {
            walker->mApplyNext = gDevice->gModifierApplyStack;
            gDevice->gModifierApplyStack = walker;
            walker = walker->mNext;
        }
    }
}

void float_to_bitmap(Device *gDevice)
{
    int i;
    for (i = 0; i < gDevice->mXRes * gDevice->mYRes; i++)
    {
        float r = gBitmapProcFloat[i * 3 + 0];
        float g = gBitmapProcFloat[i * 3 + 1];
        float b = gBitmapProcFloat[i * 3 + 2];

        gBitmapProc[i] = float_to_color(r, g, b) | 0xff000000;
    }
}

void process_image(Device *gDevice)
{
    build_applystack(gDevice);

    bitmap_to_float(gBitmapOrig, gDevice);

    Modifier *walker = gDevice->gModifierApplyStack;
    while (walker)
    {
        if (walker->mEnabled)
            walker->process(gDevice);
        walker = walker->mApplyNext;
    }

    float_to_bitmap(gDevice);
}

void img2spec_process_image(Device *gDevice)
{
    process_image(gDevice);
    gDevice->filter();
}

void img2spec_free_device(Device* device)
{
    Modifier *walker = device->gModifierRoot;
    while (walker)
    {
        Modifier *last = walker;
        walker = walker->mNext;
        delete last;
    }
    device->gModifierRoot = NULL;
    device->gModifierApplyStack = NULL;
    delete device;
}

Device* img2spec_allocate_device(int deviceId)
{
    switch (deviceId)
    {
        case 0:
            return new ZXSpectrumDevice;
        case 1:
            return new ZX3x64Device;
        case 2:
            return new ZXHalfTileDevice;
        case 3:
            return new C64HiresDevice;
        case 4:
            return new C64MulticolorDevice;
        default:
            return nullptr;
    };

    return nullptr;
}

void img2spec_zx_spectrum_set_screen_order(Device *zx, int order)
{
    static_cast<ZXSpectrumDevice*>(zx)->mOptScreenOrder = order;
}

extern void img2spec_zx_spectrum_set_screen_size(Device *zx, int w, int h)
{
    ZXSpectrumDevice* speccy = static_cast<ZXSpectrumDevice*>(zx);

    speccy->mOptWidthCells = w;
    speccy->mOptHeightCells = h;
    speccy->mXRes = speccy->mOptWidthCells * 8;
    speccy->mYRes = speccy->mOptHeightCells * (8 >> speccy->mOptCellSize);
}

void img2spec_set_scale(Device *device, float scale)
{
    Modifier *walker = device->gModifierRoot;
    while (walker)
    {
        ScalePosModifier* pos = dynamic_cast<ScalePosModifier*>(walker);
        if (pos)
        {
            pos->mScale = scale;
            break;
        }

        walker = walker->mNext;
    }
}

void img2spec_generate_result(Device *device, device_save_function_t cb)
{
    device->savefun(cb);
}