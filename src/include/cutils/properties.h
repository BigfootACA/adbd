#ifndef __CUTILS_PROPERTIES_H
#define __CUTILS_PROPERTIES_H
#define PROPERTY_KEY_MAX 32
#define PROPERTY_VALUE_MAX 92
extern int property_get(const char*key,char*value,const char*default_value);
extern int property_set(const char*key,const char*value);
extern int property_list(void(*propfn)(const char*key,const char*value,void*cookie),void*cookie);    
#endif
