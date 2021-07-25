#ifndef IMG2SPEC_DEVICE_H
#define IMG2SPEC_DEVICE_H

#include <functional>
typedef struct json_object_t JSON_Object;

class Modifier;
typedef std::function<void(unsigned char *source, uint32_t len)> device_save_function_t;

class Device
{
public:
    virtual ~Device()
    {
#ifdef WITH_GUI
        gDirty = 1;
		gDirtyPic = 1;
#endif
    }
    int mXRes;
    int mYRes;
    virtual void filter() = 0;
    virtual int estimate_rgb(int c) = 0;
    virtual void savescr(FILE * f) = 0;
    virtual void savefun(device_save_function_t cb) = 0;
    virtual void saveh(FILE * f) = 0;
    virtual void saveinc(FILE * f) = 0;
    virtual void attr_bitm() = 0;
#ifdef WITH_GUI
    virtual void options() = 0;
	virtual void zoomed(int aWhich) = 0;
#endif
    virtual void writeOptions(JSON_Object *root) = 0;
    virtual void readOptions(JSON_Object *root) = 0;
    virtual char *getname() = 0;

    Modifier *gModifierRoot = nullptr;
    Modifier *gModifierApplyStack = nullptr;
};

#endif